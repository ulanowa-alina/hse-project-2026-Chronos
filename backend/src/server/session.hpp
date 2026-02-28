#ifndef SERVER_SESSION_HPP
#define SERVER_SESSION_HPP

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <memory>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
  public:
    explicit Session(tcp::socket socket);
    void run();

  private:
    void doRead();
    void handleRequest();
    void sendResponse(http::response<http::string_body> res);

    tcp::socket socket_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
};

#endif // SERVER_SESSION_HPP
