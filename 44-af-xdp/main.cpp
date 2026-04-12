extern "C" {
#include <arpa/inet.h>
#include <bpf/bpf.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/ip_icmp.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <xdp/libxdp.h>
#include <xdp/xsk.h>
}

#include <argparse/argparse.hpp>
#include <boost/scope_exit.hpp>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <signal.h>
#include <stdexcept>
#include <string>
#include <utility>

#define FRAME_SIZE XSK_UMEM__DEFAULT_FRAME_SIZE
#define RING_SIZE XSK_RING_CONS__DEFAULT_NUM_DESCS
#define PER_XSK_FRAME_NUM (4 * RING_SIZE)
#define BATCH_SIZE 64
#define INVALID_UMEM_FRAME UINT64_MAX

struct main_config {
  std::string interface;
  std::string filename;
};

struct xsk_umem_info {
  struct xsk_ring_prod fq;
  struct xsk_ring_cons cq;
  struct xsk_umem *umem;
  void *buffer;
};

struct xsk_socket_info {
  struct xsk_ring_cons rx;
  struct xsk_ring_prod tx;
  struct xsk_ring_prod fill;
  struct xsk_ring_cons comp;
  struct xsk_umem_info *umem;
  struct xsk_socket *xsk;

  uint64_t umem_frame_addr[PER_XSK_FRAME_NUM];
  uint32_t umem_frame_free;
};

static struct main_config main_config;
static volatile bool keep_running = true;

int interface_queue_count_query(const std::string &ifname) {
  // ethtool --show-channels <ifname>

  struct ethtool_channels ec = {};
  ec.cmd = ETHTOOL_GCHANNELS;

  struct ifreq ifr = {};
  ifr.ifr_data = (__caddr_t)&ec;
  snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname.c_str());

  const int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    throw std::runtime_error("socket(AF_INET, SOCK_DGRAM) failed: " +
                             std::string(strerror(errno)));
  }
  BOOST_SCOPE_EXIT(&fd) { close(fd); }
  BOOST_SCOPE_EXIT_END

  const int ret = ioctl(fd, SIOCETHTOOL, &ifr);
  if (ret < 0) {
    throw std::runtime_error(
        "ioctl(SIOCETHTOOL, ETHTOOL_GCHANNELS) failed for " + ifname + ": " +
        std::string(strerror(errno)));
  }

  if (ec.combined_count > 0) {
    return ec.combined_count;
  }

  // Drivers may expose separate RX/TX queues without combined channels.
  // if (ec.rx_count > 0 || ec.tx_count > 0) {
  //   return std::max(ec.rx_count, ec.tx_count);
  // }

  throw std::runtime_error("failed to get queue count for interface: " +
                           ifname);
}

class xdp_attachment final {
public:
  xdp_attachment(const xdp_attachment &) = delete;
  xdp_attachment &operator=(const xdp_attachment &) = delete;

  ~xdp_attachment() { detach_noexcept(); }

  static xdp_attachment attach(const std::string &obj_path, int ifindex) {
    xdp_program *prog =
        xdp_program__open_file(obj_path.c_str(), "xdp", nullptr);
    if (!prog) {
      throw std::runtime_error("failed to open xdp program from: " + obj_path);
    }

    {
      struct xdp_multiprog *mp = xdp_multiprog__get_from_ifindex(ifindex);
      if (!mp) {
        throw std::runtime_error("failed to get xdp_multiprog for ifindex=" +
                                 std::to_string(ifindex));
      }
      struct xdp_program *tmp_prog = NULL;
      while ((tmp_prog = xdp_multiprog__next_prog(tmp_prog, mp))) {
        if (std::string(xdp_program__name(tmp_prog)) ==
            std::string(xdp_program__name(prog))) {
          enum xdp_attach_mode mode = xdp_multiprog__attach_mode(mp);
          int err = xdp_program__detach(tmp_prog, ifindex, mode, 0);
          if (err != 0) {
            throw std::runtime_error("failed to detach xdp program: " +
                                     std::string(strerror(errno)));
          }
        }
      }
    }

    // for (xdp_attach_mode mode : {XDP_MODE_NATIVE, XDP_MODE_SKB}) {
    for (xdp_attach_mode mode : {XDP_MODE_SKB}) {
      int rc = xdp_program__attach(prog, ifindex, mode, 0);
      if (rc == 0) {
        return xdp_attachment(prog, ifindex, mode);
      }
    }

    xdp_program__close(prog);
    throw std::runtime_error(
        "failed to attach XDP program to ifindex=" + std::to_string(ifindex) +
        ": " + std::string(strerror(errno)));
  }

  int xdp_map_fd_get(const char *map_name) const {
    struct bpf_map *map =
        bpf_object__find_map_by_name(xdp_program__bpf_obj(prog_), map_name);
    return bpf_map__fd(map);
  }

private:
  xdp_attachment(xdp_program *prog, int ifindex, xdp_attach_mode mode)
      : prog_(prog), ifindex_(ifindex), mode_(mode) {}

  void detach_noexcept() noexcept {
    if (!prog_ || ifindex_ == 0)
      return;
    (void)xdp_program__detach(prog_, ifindex_, mode_, 0);
    xdp_program__close(prog_);
    prog_ = nullptr;
    ifindex_ = 0;
  }

  xdp_program *prog_{nullptr};
  int ifindex_{0};
  xdp_attach_mode mode_{XDP_MODE_NATIVE};
};

static struct xsk_umem_info *configure_xsk_umem(int queue_count) {
  uint64_t num_frames = queue_count * PER_XSK_FRAME_NUM;
  DECLARE_LIBXDP_OPTS(xsk_umem_opts, opts, .size = num_frames * FRAME_SIZE,
                      .fill_size = RING_SIZE, .comp_size = RING_SIZE,
                      .frame_size = FRAME_SIZE,
                      .frame_headroom = XDP_PACKET_HEADROOM, );

  void *buffer = aligned_alloc(getpagesize(), opts.size);
  if (buffer == nullptr) {
    throw std::runtime_error("failed to alloc xsk umem: " +
                             std::string(strerror(errno)));
  }

  struct xsk_umem_info *umem = (struct xsk_umem_info *)calloc(1, sizeof(*umem));
  if (umem == nullptr) {
    throw std::runtime_error("failed to alloc xsk_umem_info: " +
                             std::string(strerror(errno)));
  }

  umem->buffer = buffer;

  umem->umem = xsk_umem__create_opts(buffer, &umem->fq, &umem->cq, &opts);
  if (umem->umem == nullptr) {
    throw std::runtime_error("failed to create umem: " +
                             std::string(strerror(errno)));
  }
  return umem;
}

static uint64_t xsk_alloc_umem_frame(struct xsk_socket_info *xsk) {
  uint64_t frame;
  if (xsk->umem_frame_free == 0) {
    throw std::runtime_error("umem frame free is 0");
  }

  frame = xsk->umem_frame_addr[--xsk->umem_frame_free];
  xsk->umem_frame_addr[xsk->umem_frame_free] = INVALID_UMEM_FRAME;
  return frame;
}

static void xsk_free_umem_frame(struct xsk_socket_info *xsk, uint64_t frame) {
  assert(xsk->umem_frame_free < PER_XSK_FRAME_NUM);
  xsk->umem_frame_addr[xsk->umem_frame_free++] = frame;
}

static uint64_t xsk_umem_free_frames(struct xsk_socket_info *xsk) {
  return xsk->umem_frame_free;
}

static struct xsk_socket_info *
configure_xsk_socket(const char *ifname, unsigned int queue_id,
                     struct xsk_umem_info *umem, xdp_attachment &attachment) {
  struct xsk_socket_info *xsk =
      (struct xsk_socket_info *)calloc(1, sizeof(*xsk));
  if (xsk == nullptr) {
    throw std::runtime_error("failed to alloc xsk_socket_info: " +
                             std::string(strerror(errno)));
  }

  DECLARE_LIBXDP_OPTS(xsk_socket_opts, socket_opts, .rx = &xsk->rx,
                      .tx = &xsk->tx, .fill = &xsk->fill, .comp = &xsk->comp,
                      .rx_size = RING_SIZE, .tx_size = RING_SIZE,
                      .libxdp_flags = XSK_LIBXDP_FLAGS__INHIBIT_PROG_LOAD, );

  xsk->umem = umem;
  xsk->xsk =
      xsk_socket__create_opts(ifname, queue_id, umem->umem, &socket_opts);
  if (xsk->xsk == nullptr) {
    throw std::runtime_error("failed to create xsk_socket: " +
                             std::string(strerror(errno)));
  }

  int ret = xsk_socket__update_xskmap(xsk->xsk,
                                      attachment.xdp_map_fd_get("xsks_map"));
  if (ret) {
    throw std::runtime_error("failed to update xsk map: " +
                             std::string(strerror(errno)));
  }

  /* Initialize umem frame allocation */
  const uint32_t per_xsk_num_descs = PER_XSK_FRAME_NUM;
  const uint32_t frame_size = FRAME_SIZE;
  const uint32_t start_frame = queue_id * per_xsk_num_descs;

  for (int i = 0; i < per_xsk_num_descs; i++) {
    xsk->umem_frame_addr[i] = (start_frame + i) * frame_size;
  }
  xsk->umem_frame_free = per_xsk_num_descs;

  // Stuff the receive path with buffers, so that we can receive packets.
  uint32_t idx = 0;
  uint64_t addr;
  int ring_size = RING_SIZE;
  ret = xsk_ring_prod__reserve(&xsk->fill, ring_size, &idx);
  while (ret != ring_size) {
    ret = xsk_ring_prod__reserve(&xsk->fill, ring_size, &idx);
  }

  for (uint32_t i = 0; i < ring_size; i++) {
    *xsk_ring_prod__fill_addr(&xsk->fill, idx++) = xsk_alloc_umem_frame(xsk);
  }

  xsk_ring_prod__submit(&xsk->fill, ring_size);

  return xsk;
}

static void hex_dump(void *pkt, size_t length, uint64_t addr) {
  const unsigned char *address = (unsigned char *)pkt;
  const unsigned char *line = address;
  size_t line_size = 32;
  unsigned char c;
  char buf[32];
  int i = 0;

  sprintf(buf, "addr=%llu", addr);
  printf("length = %zu\n", length);
  printf("%s | ", buf);
  while (length-- > 0) {
    printf("%02X ", *address++);
    if (!(++i % line_size) || (length == 0 && i % line_size)) {
      if (length == 0) {
        while (i++ % line_size)
          printf("__ ");
      }
      printf(" | "); /* right close */
      while (line < address) {
        c = *line++;
        printf("%c", (c < 33 || c == 255) ? 0x2E : c);
      }
      printf("\n");
      if (length > 0)
        printf("%s | ", buf);
    }
  }
  printf("\n");
}

static uint16_t checksum(const uint16_t *buf, size_t len) {
  uint32_t sum = 0;
  while (len > 1) {
    sum += *buf++;
    len -= 2;
  }
  if (len == 1) {
    sum += *(uint8_t *)buf;
  }
  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  return static_cast<uint16_t>(~sum);
}

static void complete_tx(struct xsk_socket_info *xsk) {

  sendto(xsk_socket__fd(xsk->xsk), NULL, 0, MSG_DONTWAIT, NULL, 0);

  /* Collect/free completed TX buffers */
  uint32_t idx_cq;
  uint32_t completed = xsk_ring_cons__peek(&xsk->comp, BATCH_SIZE, &idx_cq);

  if (completed > 0) {
    for (int i = 0; i < completed; i++) {
      uint64_t addr = *xsk_ring_cons__comp_addr(&xsk->comp, idx_cq++);
      xsk_free_umem_frame(xsk, addr);
    }

    xsk_ring_cons__release(&xsk->comp, completed);
  }
}

static void process_packet(struct xsk_socket_info *xsk, uint64_t addr,
                           uint32_t len) {
  void *pkt = xsk_umem__get_data(xsk->umem->buffer, addr);

  struct ethhdr *eth = (struct ethhdr *)pkt;
  if (ntohs(eth->h_proto) != ETH_P_IP) {
    return;
  }

  struct iphdr *ip = (struct iphdr *)(eth + 1);
  if (ip->protocol != IPPROTO_ICMP) {
    return;
  }

  struct icmphdr *icmp = (struct icmphdr *)((uint8_t *)ip + ip->ihl * 4);
  if (icmp->type != ICMP_ECHO) {
    return;
  }

  unsigned char mac_tmp[6];
  memcpy(mac_tmp, eth->h_dest, 6);
  memcpy(eth->h_dest, eth->h_source, 6);
  memcpy(eth->h_source, mac_tmp, 6);
#if 1
  uint32_t ip_tmp = ip->saddr;
  ip->saddr = ip->daddr;
  ip->daddr = ip_tmp;

  // Recalculate IP header checksum
  ip->check = 0;
  ip->check = checksum(reinterpret_cast<uint16_t *>(ip), ip->ihl * 4);

  icmp->type = ICMP_ECHOREPLY;
  icmp->checksum = 0;
  uint16_t ip_total_len = ntohs(ip->tot_len);
  uint16_t icmp_len = ip_total_len - ip->ihl * 4;
  icmp->checksum = checksum(reinterpret_cast<uint16_t *>(icmp), icmp_len);
#endif
  uint32_t tx_idx = 0;
  int reserve_ret = xsk_ring_prod__reserve(&xsk->tx, 1, &tx_idx);
  if (reserve_ret != 1) {
    /* No more transmit slots, drop the packet */
    return;
  }

  struct xdp_desc *desc = xsk_ring_prod__tx_desc(&xsk->tx, tx_idx);
  desc->addr = addr;
  desc->len = len;
  xsk_ring_prod__submit(&xsk->tx, 1);

  complete_tx(xsk);
}

static void handle_receive_packets(struct xsk_socket_info *xsk) {
  uint32_t idx_rx = 0;
  uint32_t rcvd = xsk_ring_cons__peek(&xsk->rx, BATCH_SIZE, &idx_rx);
  if (rcvd == 0)
    return;

  /* Stuff the ring with as much frames as possible */
  int stock_frames = xsk_prod_nb_free(&xsk->fill, xsk_umem_free_frames(xsk));
  if (stock_frames > 0) {
    uint32_t idx_fq = 0;
    int ret = xsk_ring_prod__reserve(&xsk->fill, stock_frames, &idx_fq);

    while (ret != stock_frames)
      ret = xsk_ring_prod__reserve(&xsk->fill, rcvd, &idx_fq);

    for (int i = 0; i < stock_frames; i++)
      *xsk_ring_prod__fill_addr(&xsk->fill, idx_fq++) =
          xsk_alloc_umem_frame(xsk);

    xsk_ring_prod__submit(&xsk->fill, stock_frames);
  }

  for (uint32_t i = 0; i < rcvd; i++) {
    const struct xdp_desc *desc = xsk_ring_cons__rx_desc(&xsk->rx, idx_rx++);
    uint64_t addr = desc->addr;
    uint32_t len = desc->len;

    void *pkt = xsk_umem__get_data(xsk->umem->buffer, addr);

    process_packet(xsk, addr, len);
    hex_dump(pkt, len, addr);

    xsk_free_umem_frame(xsk, addr);
  }

  xsk_ring_cons__release(&xsk->rx, rcvd);
}

static void signal_handler(int signum) {
  std::cerr << "Received signal: " << signum << std::endl;
  keep_running = false;
}

void rx_and_process(std::vector<struct xsk_socket_info *> &xsk_vec) {
  int nfds = xsk_vec.size();
  struct pollfd fds[nfds];

  for (int i = 0; i < nfds; i++) {
    fds[i].fd = xsk_socket__fd(xsk_vec[i]->xsk);
    fds[i].events = POLLIN;
  }

  while (keep_running) {
    int ret = poll(fds, nfds, 1000);
    if (ret <= 0) {
      continue;
    }

    for (int i = 0; i < nfds; i++) {
      if (fds[i].revents & POLLIN) {
        handle_receive_packets(xsk_vec[i]);
      }
    }
  }

  // Clean up resources
  for (auto xsk : xsk_vec) {
    xsk_socket__delete(xsk->xsk);
    free(xsk);
  }
  xsk_vec.clear();
}

int main(int argc, char *argv[]) {
  argparse::ArgumentParser argparse("af-xdp");

  argparse.add_argument("-i", "--interface")
      .required()
      .store_into(main_config.interface)
      .help("The network interface to attach the XDP program");

  argparse.add_argument("--filename")
      .required()
      .store_into(main_config.filename)
      .help("The filename of the XDP program");

  try {
    argparse.parse_args(argc, argv);

    // get queue count
    int queue_count = interface_queue_count_query(main_config.interface);
    std::cout << "interface "
              << main_config.interface << " queue count: " << queue_count
              << std::endl;

    const unsigned int ifindex = if_nametoindex(main_config.interface.c_str());
    if (ifindex == 0) {
      throw std::runtime_error("if_nametoindex failed for " +
                               main_config.interface + ": " +
                               std::string(strerror(errno)));
    }

    // attach xdp prog
    auto attachment =
        xdp_attachment::attach(main_config.filename, static_cast<int>(ifindex));

    // create umem
    struct xsk_umem_info *umem = configure_xsk_umem(queue_count);

    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGABRT, signal_handler);

    // create xsk for intf per-queue
    std::vector<struct xsk_socket_info *> xsk_vec;
    for (unsigned int i = 0; i < queue_count; i++) {
      struct xsk_socket_info *xsk = configure_xsk_socket(
          main_config.interface.c_str(), i, umem, attachment);
      xsk_vec.push_back(xsk);
    }

    rx_and_process(xsk_vec);

    // Clean up UMEM
    xsk_umem__delete(umem->umem);
    free(umem->buffer);
    free(umem);

  } catch (const std::exception &err) {
    std::cerr << err.what() << std::endl;
    return EXIT_FAILURE;
  }
}