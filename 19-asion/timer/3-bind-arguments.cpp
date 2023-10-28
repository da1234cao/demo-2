#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>

void print(const boost::system::error_code & /*e*/,
           boost::shared_ptr<boost::asio::steady_timer> &tp, int &count) {
  if (count < 5) {
    std::cout << count++ << std::endl;
    tp->expires_at(tp->expiry() + boost::asio::chrono::seconds(1));
    tp->async_wait(
        boost::bind(print, boost::asio::placeholders::error, tp, count));
  }
}

int main(int argc, char *argv[]) {
  int count = 0;
  boost::asio::io_context io;
  boost::shared_ptr<boost::asio::steady_timer> tp(
      new boost::asio::steady_timer(io, boost::asio::chrono::seconds(1)));
  tp->async_wait(boost::bind(print, boost::placeholders::_1, tp, count));
  io.run();
  return 0;
}