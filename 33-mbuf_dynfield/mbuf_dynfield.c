#include <rte_eal.h>
#include <rte_errno.h>
#include <rte_ethdev.h>
#include <rte_log.h>
#include <rte_mbuf.h>

#define RTE_LOGTYPE_DYNFIELD RTE_LOGTYPE_USER1
#define NUM_MBUFS 128
#define MBUF_CACHE_SIZE 64

int main(int argc, char *argv[]) {
  if (rte_eal_init(argc, argv) < 0) {
    RTE_LOG(ERR, DYNFIELD, "%s\n", rte_strerror(rte_errno));
    exit(-1);
  }

  int nb_ports = rte_eth_dev_count_avail();
  if (nb_ports < 2 || (nb_ports & 1)) {
    RTE_LOG(ERR, DYNFIELD, "%s\n", "number of ports must be even\n");
    exit(-1);
  }

  // 对齐要求: priv_size 必须是8的整数倍
  struct rte_mempool *pool = rte_pktmbuf_pool_create(
      "MBUF_POOL", NUM_MBUFS * nb_ports, MBUF_CACHE_SIZE, 8,
      RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

  if (pool == NULL) {
    RTE_LOG(ERR, DYNFIELD, "%s\n", rte_strerror(rte_errno));
    exit(-1);
  }

#if 0
  struct rte_mbuf *tmp = rte_pktmbuf_alloc(pool);
  assert(rte_mbuf_to_priv(tmp) ==
         (void *)((uintptr_t)tmp + sizeof(struct rte_mbuf)));
#endif
}