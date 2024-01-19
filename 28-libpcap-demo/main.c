#include <netinet/ether.h>
#include <netinet/ip.h>
#include <pcap.h>
#include <stdlib.h>
#include <string.h>

#define DEVICE_NAME "enp0s3"
#define PACKAGE_CAPTURE_COUNT 2
#define PCAP_DUMP_FILE_PATH "./baidu.pcap"

int enum_dev() {
  // 查看当前有哪些网络设备
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_if_t *alldevs, *dev;

  // 获取可用的网络设备列表
  if (pcap_findalldevs(&alldevs, errbuf) == -1) {
    fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
    return -1;
  }

  int count = 1;
  struct sockaddr_in in;
  // 遍历设备列表并打印信息
  for (dev = alldevs; dev != NULL; dev = dev->next) {
    printf("Device %d: %s\n", count++, dev->name);
    if (dev->description) {
      printf("   Description: %s\n", dev->description);
    }
    for (pcap_addr_t *address = dev->addresses; address != NULL;
         address = address->next) {
      char addr_str[INET6_ADDRSTRLEN];
      const struct sockaddr *sa = address->addr;
      if (sa) {
        char addr_str[INET6_ADDRSTRLEN];
        if (sa->sa_family == AF_INET) {
          struct sockaddr_in *ipv4 = (struct sockaddr_in *)sa;
          inet_ntop(AF_INET, &(ipv4->sin_addr), addr_str, sizeof(addr_str));
          printf("  IPv4 Address: %s\n", addr_str);
        } else if (sa->sa_family == AF_INET6) {
          struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)sa;
          inet_ntop(AF_INET6, &(ipv6->sin6_addr), addr_str, sizeof(addr_str));
          printf("  IPv6 Address: %s\n", addr_str);
        }
      }
    }
  }

  // 释放设备列表
  pcap_freealldevs(alldevs);
  printf("\n\n");
  return 0;
}

int print_device_net() {
  // 查看dev的 network number和子网掩码
  // ipv4变量中并非ip值，而是 network number
  // ref: https://github.com/the-tcpdump-group/libpcap/issues/159
  char errbuf[PCAP_ERRBUF_SIZE];
  int ret;
  bpf_u_int32 netp;
  bpf_u_int32 maskp;
  char ipv4[INET_ADDRSTRLEN];
  char ipv4_mask[INET_ADDRSTRLEN];
  ret = pcap_lookupnet(DEVICE_NAME, &netp, &maskp, errbuf);
  if (ret < 0) {
    fprintf(stderr, "fail in pcap_lookupnet: %s", errbuf);
    return -1;
  }
  inet_ntop(AF_INET, &netp, ipv4, sizeof(ipv4));
  inet_ntop(AF_INET, &maskp, ipv4_mask, sizeof(ipv4_mask));
  printf("%s network address is: %s\n", DEVICE_NAME, ipv4);
  printf("%s mask is: %s\n", DEVICE_NAME, ipv4_mask);
  printf("\n\n");
  return 0;
}

void print_mac_address(const u_int8_t *mac_address, char *mac_string,
                       unsigned int size) {
  snprintf(mac_string, size, "%02X:%02X:%02X:%02X:%02X:%02X", mac_address[0],
           mac_address[1], mac_address[2], mac_address[3], mac_address[4],
           mac_address[5]);
}

void print_packet_info(struct pcap_pkthdr packet_header, const u_char *packet) {
  printf("Packet capture length: %d\n", packet_header.caplen);
  printf("Packet total length %d\n", packet_header.len);

  // todo: 从const u_char *packet中，逐层解析
  char mac[ETH_HLEN + 5 + 1];
  struct ether_header *eth_header = (struct ether_header *)packet;
  print_mac_address(eth_header->ether_shost, mac, sizeof(mac));
  printf("src mac addr is: %s\n", mac);
  print_mac_address(eth_header->ether_dhost, mac, sizeof(mac));
  printf("dst mac addr is: %s\n", mac);

  if (ntohs(eth_header->ether_type) == ETHERTYPE_IP) {
    struct iphdr *ip_header =
        (struct iphdr *)(packet + sizeof(struct ether_header));
    char ipv4[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip_header->saddr, ipv4, sizeof(ipv4));
    printf("src ip addr is: %s\n", ipv4);
    inet_ntop(AF_INET, &ip_header->daddr, ipv4, sizeof(ipv4));
    printf("dst ip addr is: %s\n", ipv4);
  }
}

void capture_packet_handler(u_char *args,
                            const struct pcap_pkthdr *packet_header,
                            const u_char *packet_body) {
  print_packet_info(*packet_header, packet_body);
  pcap_dump(args, packet_header, packet_body);
  return;
}

void capture() {
  // 捕获数据包,并保存到文件
  int ret;
  char errbuf[PCAP_ERRBUF_SIZE];

  pcap_t *pcap_handle;
  struct bpf_program filter;
  pcap_handle = pcap_create(DEVICE_NAME, errbuf);
  if (pcap_handle == NULL) {
    fprintf(stderr, "fail to execute pcap_create: %s", errbuf);
    exit(-1);
  }

  // https://stackoverflow.com/questions/29875110/why-is-there-a-long-delay-between-pcap-loop-and-getting-a-packet
  pcap_set_timeout(pcap_handle, 1000);

  ret = pcap_activate(pcap_handle);

  if (pcap_compile(pcap_handle, &filter, "dst host www.baidu.com", 0,
                   PCAP_NETMASK_UNKNOWN) < 0) {
    fprintf(stderr, "fail to compile bpf rule: %s\n", pcap_geterr(pcap_handle));
    exit(-1);
  }
  if (ret < 0) {
    fprintf(stderr, "fail to active pcap hanlde: %s", pcap_geterr(pcap_handle));
    exit(-1);
  }
  if (pcap_setfilter(pcap_handle, &filter) < 0) {
    printf("Error setting filter - %s\n", pcap_geterr(pcap_handle));
    exit(-1);
  }

  pcap_dumper_t *pacp_dump_point =
      pcap_dump_open(pcap_handle, PCAP_DUMP_FILE_PATH);
  if (pacp_dump_point == NULL) {
    printf("fail to open dump file - %s\n", pcap_geterr(pcap_handle));
    exit(-1);
  }

  pcap_loop(pcap_handle, PACKAGE_CAPTURE_COUNT, capture_packet_handler,
            (u_char *)pacp_dump_point);

  pcap_dump_flush(pacp_dump_point);
  printf("\n\nsaved package to %s\n\n", PCAP_DUMP_FILE_PATH);

  pcap_dump_close(pacp_dump_point);
  pcap_close(pcap_handle);
}

void load_packet_handler(u_char *args, const struct pcap_pkthdr *packet_header,
                         const u_char *packet_body) {
  print_packet_info(*packet_header, packet_body);
  return;
}

void load_pcap_file() {
  printf("load %s file.\n", PCAP_DUMP_FILE_PATH);
  char errbuf[PCAP_ERRBUF_SIZE];

  pcap_t *pcap_handle = pcap_open_offline(PCAP_DUMP_FILE_PATH, errbuf);
  if (pcap_handle == NULL) {
    fprintf(stderr, "fail to execute pcap_open_offline: %s", errbuf);
    exit(-1);
  }
  pcap_activate(pcap_handle);
  pcap_loop(pcap_handle, PACKAGE_CAPTURE_COUNT, load_packet_handler, NULL);
}

int main(int argc, char *argv[]) {
  capture();
  load_pcap_file();
}