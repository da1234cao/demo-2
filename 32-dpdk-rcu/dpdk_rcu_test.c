#include <signal.h>
#include <stdatomic.h>

#include <rte_common.h>
#include <rte_eal.h>
#include <rte_errno.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_memcpy.h>
#include <rte_rcu_qsbr.h>
#include <rte_thread.h>

#define RTE_LOGTYPE_RCU_TEST RTE_LOGTYPE_USER1
#define CONFIG_SIZE 100

struct rte_rcu_qsbr *rcu = NULL;
char *global_config = NULL;

void signal_handle(int signum) {
  if (signum == SIGTERM) {
    exit(0);
  } else if (signum == SIGHUP) {
    char *new_config = rte_zmalloc(NULL, CONFIG_SIZE, 0);
    if (strcmp(global_config, "enable") == 0) {
      snprintf(new_config, CONFIG_SIZE, "%s", "disable");
    } else {
      snprintf(new_config, CONFIG_SIZE, "%s", "enable");
    }

    // 使用新的内存地址，替换旧的内存地址
    char *old_config = global_config;
    global_config = new_config;
    // __atomic_store(&global_config, &new_config, __ATOMIC_RELAXED);

    // 阻塞,知道所有reader在旧地址上的读取结束
    rte_rcu_qsbr_synchronize(rcu, RTE_QSBR_THRID_INVALID);

    // 释放旧内存
    rte_free(old_config);
  } else {
    RTE_LOG(DEBUG, RCU_TEST, "unsolve signal: %d\n", signum);
  }
}

uint32_t loop(void *arg) {

  // can not lcore id
  // unsigned int lcore_id = rte_lcore_id();

  unsigned int lcore_id = *(int *)arg;

  RTE_LOG(DEBUG, RCU_TEST, "start thread %u\n", lcore_id);

  rte_rcu_qsbr_thread_register(rcu, lcore_id);

  while (1) {
    rte_rcu_qsbr_thread_online(rcu, lcore_id);
    RTE_LOG(DEBUG, RCU_TEST, "curret config: %s\n", global_config);
    rte_rcu_qsbr_quiescent(rcu, lcore_id);
    rte_rcu_qsbr_thread_offline(rcu, lcore_id);
  }

  return 0;
}

int main(int argc, char *argv[]) {

  rte_log_set_level(RTE_LOGTYPE_RCU_TEST, RTE_LOG_DEBUG);
  RTE_LOG(DEBUG, RCU_TEST, "%s\n", "start dpdk_rcu_test...");

  if (rte_eal_init(argc, argv) < 0) {
    RTE_LOG(ERR, RCU_TEST, "%s\n", rte_strerror(rte_errno));
    exit(-1);
  }

  size_t size = rte_rcu_qsbr_get_memsize(RTE_MAX_LCORE);
  rcu = rte_zmalloc(NULL, size, 0);
  rte_rcu_qsbr_init(rcu, RTE_MAX_LCORE);

  global_config = rte_zmalloc(NULL, CONFIG_SIZE, 0);
  if (global_config == NULL) {
    RTE_LOG(ERR, RCU_TEST, "%s\n", "no enough space");
    exit(-1);
  }

  char status[] = "enable";
  rte_memcpy(global_config, status, sizeof(status));

  // 注册信号处理函数
  struct sigaction action;
  action.sa_handler = signal_handle;
  action.sa_flags = SA_RESTART;
  sigfillset(&action.sa_mask);
  sigaction(SIGTERM, &action, NULL);
  sigaction(SIGHUP, &action, NULL);

  // 创建多个reader线程
  unsigned int lcore_cnt = rte_lcore_count();
  rte_thread_t thread_ids[lcore_cnt];
  for (unsigned int i = 0; i < lcore_cnt; i++) {
    rte_thread_create(&thread_ids[i], NULL, loop, &i);
  }

  // 避免进程退出
  for (unsigned int i = 0; i < lcore_cnt; i++) {
    rte_thread_join(thread_ids[i], NULL);
  }

  rte_free(global_config);
}