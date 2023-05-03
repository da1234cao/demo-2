#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <iostream>
#include <string>

class req_info {
public:
  std::string host;
  std::string service;
  std::string target;
  boost::beast::http::verb method;
};

class request {
public:
  request() = default;
  request(req_info info_) : info(info_) {
    IS_HTTPS = info.service == "https" ? true : false;
  }

  void get_response() {
    if (IS_HTTPS) {
      https_get();
    } else {
      http_get();
    }
  }

private:
  void http_get() {
    try {
      req.method(info.method);
      req.target(info.target);
      req.set(boost::beast::http::field::host, info.host);
      req.set(boost::beast::http::field::user_agent, "boost beast test");
      boost::asio::io_context ioc;
      boost::asio::ip::tcp::resolver resolver(ioc);
      auto results = resolver.resolve(info.host, info.service);

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

  void https_get() {
    try {
      boost::asio::io_context ioc;
      boost::asio::ssl::context ctx(boost::asio::ssl::context::tlsv12_client);
      ctx.set_verify_mode(boost::asio::ssl::verify_none);
      boost::asio::ip::tcp::resolver resolver(ioc);
      auto results = resolver.resolve(info.host, info.service);

      boost::beast::ssl_stream<boost::beast::tcp_stream> stream(ioc, ctx);
      // Set SNI Hostname (many hosts need this to handshake successfully)
      SSL_set_tlsext_host_name(stream.native_handle(), info.host.c_str());

      boost::beast::get_lowest_layer(stream).connect(results);

      // Perform the SSL handshake
      stream.handshake(boost::asio::ssl::stream_base::client);

      req.method(info.method);
      req.target(info.target);
      req.set(boost::beast::http::field::host, info.host);
      req.set(boost::beast::http::field::user_agent, "boost beast https test");

      boost::beast::http::write(stream, req);

      boost::beast::flat_buffer buffer;
      boost::beast::http::read(stream, buffer, res);

      std::cout << "https response" << std::endl;
      std::cout << "return code: " << res.result_int() << std::endl;
      std::cout << "HTTP/" << res.version() << " " << res.result() << " "
                << res.reason() << "\n";
      std::cout << "Body: "
                << boost::beast::buffers_to_string(res.body().data()) << "\n";
      stream.shutdown();
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
  // info.service = "http";
  info.service = "https";
  info.target = "/";
  info.method = boost::beast::http::verb::get;

  request req(info);
  req.get_response();
}