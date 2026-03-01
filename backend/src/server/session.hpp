#ifndef SERVER_SESSION_HPP
#define SERVER_SESSION_HPP

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <map>
#include <memory>
#include <string>
#include <functional>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = asio::ip::tcp;

using Request  = http::request<http::string_body>;
using Response = http::response<http::string_body>;
using RequestHandler = std::function<Response(const Request&)>;
using Router = std::map<std::string, RequestHandler>;

class Session : public std::enable_shared_from_this<Session> {
  public:
    Session(tcp::socket socket, Router router);
    void run();

  private:
    void doRead();
    void handleRequest();
    void sendResponse(http::response<http::string_body> res);

    tcp::socket socket_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    Router router_;
};

#endif // SERVER_SESSION_HPP
