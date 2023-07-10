#include <csignal>
#include <iostream>

namespace {
volatile std::sig_atomic_t gSignalStatus;
}

void signal_handler(int signal) { gSignalStatus = signal; }

int main() {
  // ��װ�źŴ�����
  std::signal(SIGTERM, signal_handler);

  std::cout << "�ź�ֵ��" << gSignalStatus << '\n';
  std::cout << "�����źţ�" << SIGTERM << '\n';
  std::raise(SIGTERM);
  std::cout << "�ź�ֵ��" << gSignalStatus << '\n';
}
