#include <iostream>
#include <signal.h>

#define SIGNOTHING SIGRTMIN + 1

void signal_handle(int signum) {

  if (signum == SIGTERM) { /*终止请求信号*/
    std::cout << "receive term signal, will exit now." << std::endl;
    exit(0);
  } else if (signum == SIGUSR1) { /*用户定义信号*/
    std::cout << "receive user1 signal." << std::endl;
  } else if (signum == SIGNOTHING) { /*自定义信号*/
    std::cout << "receive nothing signal." << std::endl;
  } else {
    std::cout << "unsolve signal: " << signum << std::endl;
  }
}

int main(int argc, char *argv[]) {
  std::cout << "pid: " << getpid() << std::endl;
  std::cout << "SIGRTMIN: " << SIGRTMIN << std::endl;
  std::cout << "__SIGRTMIN: " << __SIGRTMIN << std::endl;

  struct sigaction action;
  action.sa_handler = signal_handle;
  action.sa_flags = SA_RESTART; // 被信号中断的系统调用能自动重启
  // 等信号处理函数,来的信号被加入到屏蔽字中,等待信号处理函数返回后再恢复
  sigfillset(&action.sa_mask);

  // 修改信号的默认动作-通常的默认动作是SIG_IGN,SIG_DFL
  sigaction(SIGTERM, &action, nullptr);
  sigaction(SIGUSR1, &action, nullptr);
  sigaction(SIGNOTHING, &action, nullptr);

  while (1) {
    sleep(1);
  }
}