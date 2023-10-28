#include <boost/asio.hpp>
#include <iostream>

int main(int argc, char *argv[]) {
  boost::asio::io_context io;
  boost::asio::steady_timer t(io, boost::asio::chrono::seconds(5));
  t.wait();
  std::cout << "hello world" << std::endl;
  return 0;
}