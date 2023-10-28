#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <iostream>

void print(const boost::system::error_code & /*e*/,
           boost::asio::steady_timer &t, int &count) {
  if (count < 5) {
    std::cout << count++ << std::endl;
    t.expires_at(t.expiry() + boost::asio::chrono::seconds(1));
    t.async_wait(
        boost::bind(print, boost::asio::placeholders::error, t, count));
    // t.async_wait(boost::bind(print, boost::asio::placeholders::error,
    //                          boost::ref(t), count));
  }
}

int main(int argc, char *argv[]) {
  int count = 0;
  boost::asio::io_context io;
  boost::asio::steady_timer t(io, boost::asio::chrono::seconds(1));
  t.async_wait(boost::bind(print, boost::asio::placeholders::error, t, count));
  // t.async_wait(boost::bind(print, boost::asio::placeholders::error,
  //                          boost::ref(t), count));
  io.run();
  return 0;
}