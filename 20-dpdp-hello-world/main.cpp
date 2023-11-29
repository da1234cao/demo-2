#include <rte_eal.h>
#include <rte_lcore.h>
#include <stdio.h>

int lcore_hello(__rte_unused void *arg) {
  unsigned int lcore_id = rte_lcore_id();
  printf("hello from core %u\n", lcore_id);
  return 0;
}

int main(int argc, char *argv[]) {
  if (rte_eal_init(argc, argv) < 0) {
    rte_exit(EXIT_FAILURE, "fail in init");
  }
  for (unsigned int lcore_id = rte_get_next_lcore(-1, 1, 0);
       lcore_id < RTE_MAX_LCORE;
       lcore_id = rte_get_next_lcore(lcore_id, 1, 0)) {
    rte_eal_remote_launch(lcore_hello, NULL, lcore_id);
  }

  lcore_hello(NULL);

  rte_eal_mp_wait_lcore(); // Wait until all lcores finish their jobs.

  rte_eal_cleanup();

  return 0;
}