#include <boost/asio.hpp>
#include <iostream>

int main(int argc, char *argv[]) {
  boost::asio::io_context io_context;
  boost::asio::signal_set signals(io_context);
  signals.add(SIGINT);
  signals.add(SIGTERM);
  signals.async_wait([](boost::system::error_code ec, int signo) {
    std::cout << "receive signo " << signo;
  });
  io_context.run();
  return 0;
}