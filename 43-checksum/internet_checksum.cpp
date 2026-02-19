#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

static inline uint16_t raw_cksum_reduce(uint32_t cksum) {
  while (cksum >> 16) {
    cksum = (cksum & 0xffff) + (cksum >> 16);
  }
  return (uint16_t)cksum;
}

static inline uint16_t raw_cksum(const void *addr, size_t len) {
  const uint16_t *buf = (const uint16_t *)addr;
  uint32_t cksum = 0;

  while (len > 1) {
    cksum += *buf++;
    len -= 2;
  }

  if (len > 0) {
    cksum += *(const uint8_t *)buf;
  }

  return raw_cksum_reduce(cksum);
}

static inline uint16_t ip4_header_cheraw_cksum_inline(struct iphdr *iph) {
  size_t len = iph->ihl * 4;
  uint16_t cksum = raw_cksum(iph, len);
  return cksum;
}

static inline uint16_t ip4_header_cheraw_cksum(struct iphdr *iph) {
  uint16_t cksum = ip4_header_cheraw_cksum_inline(iph);
  return ~cksum;
}

static inline bool ip4_header_cheraw_cksum_verify(struct iphdr *iph) {
  uint16_t cksum = ip4_header_cheraw_cksum_inline(iph);
  return cksum == 0xFFFF;
}

uint16_t ipv4_udptcp_raw_cksum_inline(struct iphdr *iph, void *transport_hdr) {
  struct ipv4_psd_header {
    uint32_t saddr;
    uint32_t daddr;
    uint8_t zero;
    uint8_t protocol;
    uint16_t len;
  } psd_hdr;

  memset(&psd_hdr, 0, sizeof(psd_hdr));
  psd_hdr.saddr = iph->saddr;
  psd_hdr.daddr = iph->daddr;
  psd_hdr.zero = 0;
  psd_hdr.protocol = iph->protocol;
  uint16_t l4_len = ntohs(iph->tot_len) - iph->ihl * 4;
  psd_hdr.len = htons(l4_len);

  uint16_t cksum = raw_cksum(&psd_hdr, sizeof(psd_hdr));
  cksum += raw_cksum(transport_hdr, l4_len);
  cksum = raw_cksum_reduce(cksum);

  return cksum;
}

uint16_t ipv4_udptcp_raw_cksum(struct iphdr *iph, void *transport_hdr) {
  uint16_t cksum = ipv4_udptcp_raw_cksum_inline(iph, transport_hdr);

  cksum = ~cksum;

  /*
   * Per RFC 768: If the computed cheraw_cksum is zero for UDP,
   * it is transmitted as all ones
   */
  if (cksum == 0 && iph->protocol == IPPROTO_UDP)
    cksum = 0xffff;

  return cksum;
}

uint16_t ipv4_udptcp_raw_cksum_verify(struct iphdr *iph, void *transport_hdr) {
  uint16_t cksum = ipv4_udptcp_raw_cksum_inline(iph, transport_hdr);
  return cksum == 0xFFFF;
}

uint16_t ipv6_udptcp_raw_cksum_inline(struct ip6_hdr *ip6h,
                                      void *transport_hdr) {
  uint32_t cksum = 0;

  /**
   * IPv6 pseudo-header is 320 bits.
   * It contains the source and destination IPv6 addresses,
   * the next header protocol, and the L4 length.
   */

  struct ipv6_psd_header {
    uint32_t len;   /* L4 length. */
    uint32_t proto; /* L4 protocol - top 3 bytes must be zero */
  };

  struct ipv6_psd_header psd_hdr = {};
  psd_hdr.proto = (uint32_t)(ip6h->ip6_nxt << 24); // little endian
  psd_hdr.len = ip6h->ip6_ctlun.ip6_un1.ip6_un1_plen;

  cksum =
      raw_cksum(&ip6h->ip6_src, sizeof(ip6h->ip6_src) + sizeof(ip6h->ip6_dst));
  cksum += raw_cksum(&psd_hdr, sizeof(psd_hdr));

  cksum += raw_cksum(transport_hdr, ntohs(psd_hdr.len));

  return raw_cksum_reduce(cksum);
}

uint16_t ipv6_udptcp_raw_cksum(struct ip6_hdr *ip6h, void *transport_hdr) {
  uint16_t cksum = ipv6_udptcp_raw_cksum_inline(ip6h, transport_hdr);

  cksum = ~cksum;

  /*
   * Per RFC 2460: If the computed checksum is zero for UDP,
   * it is transmitted as all ones
   */
  if (cksum == 0 && ip6h->ip6_nxt == IPPROTO_UDP)
    cksum = 0xffff;

  return cksum;
}

uint16_t ipv6_udptcp_raw_cksum_verify(struct ip6_hdr *ip6h,
                                      void *transport_hdr) {
  uint16_t cksum = ipv6_udptcp_raw_cksum_inline(ip6h, transport_hdr);
  return cksum == 0xFFFF;
}

void test_scapy_ipv4_tcp() {
  /* generated in scapy with Ether()/IP()/TCP() */
  uint8_t test_cksum_ipv4_tcp[] = {
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x08, 0x00, 0x45, 0x00, 0x00, 0x28, 0x00, 0x01, 0x00, 0x00,
      0x40, 0x06, 0x7c, 0xcd, 0x7f, 0x00, 0x00, 0x01, 0x7f, 0x00, 0x00,
      0x01, 0x00, 0x14, 0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x50, 0x02, 0x20, 0x00, 0x91, 0x7c, 0x00, 0x00,
  };

  const uint8_t *pkt = (const uint8_t *)test_cksum_ipv4_tcp;
  // size_t pkt_len = sizeof(test_cksum_ipv4_tcp);

  // struct ethhdr *eth = (struct ethhdr *)pkt;
  struct iphdr *iph = (struct iphdr *)(pkt + sizeof(struct ethhdr));
  struct tcphdr *tcph =
      (struct tcphdr *)(pkt + sizeof(struct ethhdr) + iph->ihl * 4);

  uint16_t expected_cksum = tcph->check;
  tcph->check = 0;
  uint16_t calc_cksum = ipv4_udptcp_raw_cksum(iph, tcph);
  bool valid = (calc_cksum == expected_cksum);

  std::cout << "=== Scapy IPv4 TCP Test ===" << std::endl;
  std::cout << "Expected: 0x" << std::hex << ntohs(expected_cksum) << std::dec
            << std::endl;
  std::cout << "Calculated: 0x" << std::hex << ntohs(calc_cksum) << std::dec
            << std::endl;
  std::cout << "Validation: " << (valid ? "PASS" : "FAIL") << std::endl;
  std::cout << std::endl;
}

void test_scapy_ipv6_tcp() {
  /* generated in scapy with Ether()/IPv6()/TCP()) */
  uint8_t test_cksum_ipv6_tcp[] = {
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x08, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x14, 0x06, 0x40,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
      0x14, 0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x50, 0x02, 0x20, 0x00, 0x8f, 0x7d, 0x00, 0x00,
  };

  const uint8_t *pkt = (const uint8_t *)test_cksum_ipv6_tcp;
  // struct ethhdr *eth = (struct ethhdr *)pkt;
  struct ip6_hdr *ip6h = (struct ip6_hdr *)(pkt + sizeof(struct ethhdr));
  struct tcphdr *tcph =
      (struct tcphdr *)(pkt + sizeof(struct ethhdr) + sizeof(struct ip6_hdr));

  uint16_t expected_cksum = tcph->check;
  tcph->check = 0;
  uint16_t calc_cksum = ipv6_udptcp_raw_cksum(ip6h, tcph);
  bool valid = (calc_cksum == expected_cksum);

  std::cout << "=== Scapy IPv6 TCP Test ===" << std::endl;
  std::cout << "Expected: 0x" << std::hex << ntohs(expected_cksum) << std::dec
            << std::endl;
  std::cout << "Calculated: 0x" << std::hex << ntohs(calc_cksum) << std::dec
            << std::endl;
  std::cout << "Validation: " << (valid ? "PASS" : "FAIL") << std::endl;
  std::cout << std::endl;
}

void test_scapy_ipv4_udp() {
  /* generated in scapy with Ether()/IP()/UDP()/Raw('x')) */
  uint8_t test_cksum_ipv4_udp[] = {
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x08, 0x00, 0x45, 0x00, 0x00, 0x1d, 0x00, 0x01, 0x00, 0x00,
      0x40, 0x11, 0x7c, 0xcd, 0x7f, 0x00, 0x00, 0x01, 0x7f, 0x00, 0x00,
      0x01, 0x00, 0x35, 0x00, 0x35, 0x00, 0x09, 0x89, 0x6f, 0x78,
  };

  const uint8_t *pkt = (const uint8_t *)test_cksum_ipv4_udp;
  // struct ethhdr *eth = (struct ethhdr *)pkt;
  struct iphdr *iph = (struct iphdr *)(pkt + sizeof(struct ethhdr));
  struct udphdr *udph =
      (struct udphdr *)(pkt + sizeof(struct ethhdr) + iph->ihl * 4);

  uint16_t expected_cksum = udph->check;
  udph->check = 0;
  uint16_t calc_cksum = ipv4_udptcp_raw_cksum(iph, udph);
  bool valid = (calc_cksum == expected_cksum);

  std::cout << "=== Scapy IPv4 UDP Test ===" << std::endl;
  std::cout << "Expected: 0x" << std::hex << ntohs(expected_cksum) << std::dec
            << std::endl;
  std::cout << "Calculated: 0x" << std::hex << ntohs(calc_cksum) << std::dec
            << std::endl;
  std::cout << "Validation: " << (valid ? "PASS" : "FAIL") << std::endl;
  std::cout << std::endl;
}

void test_scapy_ipv6_udp() {
  /* generated in scapy with Ether()/IPv6()/UDP()/Raw('x')) */
  uint8_t test_cksum_ipv6_udp[] = {
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x86, 0xdd, 0x60, 0x00, 0x00, 0x00, 0x00, 0x09, 0x11, 0x40,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
      0x35, 0x00, 0x35, 0x00, 0x09, 0x87, 0x70, 0x78,
  };

  const uint8_t *pkt = (const uint8_t *)test_cksum_ipv6_udp;
  // struct ethhdr *eth = (struct ethhdr *)pkt;
  struct ip6_hdr *ip6h = (struct ip6_hdr *)(pkt + sizeof(struct ethhdr));
  struct udphdr *udph =
      (struct udphdr *)(pkt + sizeof(struct ethhdr) + sizeof(struct ip6_hdr));

  uint16_t expected_cksum = udph->check;
  udph->check = 0;
  uint16_t calc_cksum = ipv6_udptcp_raw_cksum(ip6h, udph);
  bool valid = (calc_cksum == expected_cksum);

  std::cout << "=== Scapy IPv6 UDP Test ===" << std::endl;
  std::cout << "Expected: 0x" << std::hex << ntohs(expected_cksum) << std::dec
            << std::endl;
  std::cout << "Calculated: 0x" << std::hex << ntohs(calc_cksum) << std::dec
            << std::endl;
  std::cout << "Validation: " << (valid ? "PASS" : "FAIL") << std::endl;
  std::cout << std::endl;
}

void test_scapy_ipv4_opts_udp() {
  /* generated in scapy with Ether()/IP(options='\x00')/UDP()/Raw('x')) */
  uint8_t test_cksum_ipv4_opts_udp[] = {
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x08, 0x00, 0x46, 0x00, 0x00, 0x21, 0x00, 0x01, 0x00, 0x00, 0x40, 0x11,
      0x7b, 0xc9, 0x7f, 0x00, 0x00, 0x01, 0x7f, 0x00, 0x00, 0x01, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x35, 0x00, 0x35, 0x00, 0x09, 0x89, 0x6f, 0x78,
  };

  const uint8_t *pkt = (const uint8_t *)test_cksum_ipv4_opts_udp;
  // struct ethhdr *eth = (struct ethhdr *)pkt;
  struct iphdr *iph = (struct iphdr *)(pkt + sizeof(struct ethhdr));
  struct udphdr *udph =
      (struct udphdr *)(pkt + sizeof(struct ethhdr) + iph->ihl * 4);

  uint16_t expected_cksum = udph->check;
  udph->check = 0;
  uint16_t calc_cksum = ipv4_udptcp_raw_cksum(iph, udph);
  bool valid = (calc_cksum == expected_cksum);

  std::cout << "=== Scapy IPv4 Options UDP Test ===" << std::endl;
  std::cout << "Expected: 0x" << std::hex << ntohs(expected_cksum) << std::dec
            << std::endl;
  std::cout << "Calculated: 0x" << std::hex << ntohs(calc_cksum) << std::dec
            << std::endl;
  std::cout << "Validation: " << (valid ? "PASS" : "FAIL") << std::endl;
  std::cout << std::endl;
}

int main() {
  std::cout << "Internet Checksum Examples" << std::endl;
  std::cout << "==========================" << std::endl;
  std::cout << std::endl;

  test_scapy_ipv4_tcp();
  test_scapy_ipv6_tcp();
  test_scapy_ipv4_udp();
  test_scapy_ipv6_udp();
  test_scapy_ipv4_opts_udp();

  std::cout << "All tests completed!" << std::endl;

  return 0;
}
