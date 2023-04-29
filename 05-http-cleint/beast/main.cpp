#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <string>

class req_info {
public:
  std::string host;
  std::string port;
  std::string target;
  boost::beast::http::verb method;
};

class request {
public:
  request() = default;
  request(req_info info_) : info(info_) {}

  void get_response() {
    try {
      req.set(boost::beast::http::field::host, info.host);
      req.target(info.target);
      req.method(info.method);

      boost::asio::io_context ioc;
      boost::asio::ip::tcp::resolver resolver(ioc);
      auto results = resolver.resolve(info.host, info.port);

      boost::beast::tcp_stream stream(ioc);
      stream.connect(results);

      boost::beast::http::write(stream, req);

      boost::beast::flat_buffer buffer;
      boost::beast::http::read(stream, buffer, res);

      std::cout << "return code: " << res.result_int() << std::endl;
      std::cout << "HTTP/" << res.version() << " " << res.result() << " "
                << res.reason() << "\n";
      std::cout << "Body: "
                << boost::beast::buffers_to_string(res.body().data()) << "\n";
      stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    } catch (std::exception const &e) {
      std::cerr << "Error: " << e.what() << std::endl;
    }
  }

private:
  req_info info;
  bool IS_HTTPS;
  boost::beast::http::request<boost::beast::http::string_body> req;
  boost::beast::http::response<boost::beast::http::dynamic_body> res;
};

int main(int argc, char *argv[]) {
  req_info info;
  info.host = "www.baidu.com";
  info.port = "http";
  info.target = "/";
  info.method = boost::beast::http::verb::get;

  request req(info);
  req.get_response();
}