#include <csignal>
#include <iostream>

namespace {
volatile std::sig_atomic_t gSignalStatus;
}

void signal_handler(int signal) { gSignalStatus = signal; }

int main() {
  // 安装信号处理函数
  std::signal(SIGTERM, signal_handler);

  std::cout << "信号值：" << gSignalStatus << '\n';
  std::cout << "发送信号：" << SIGTERM << '\n';
  std::raise(SIGTERM);
  std::cout << "信号值：" << gSignalStatus << '\n';
}
