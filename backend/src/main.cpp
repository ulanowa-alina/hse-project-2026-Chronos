#include "server/personal/v1/info.hpp"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
  public:
    explicit Session(tcp::socket socket)
        : socket_(std::move(socket)) {
    }

    void run() {
        doRead();
    }

  private:
    void doRead() {
        req_ = {};
        http::async_read(socket_, buffer_, req_,
                         [self = shared_from_this()](beast::error_code ec, std::size_t) {
                             if (!ec)
                                 self->handleRequest();
                         });
    }

    void handleRequest() {
        if (req_.method() == http::verb::get && req_.target() == "/personal/v1/info") {
            sendResponse(personal::v1::handleInfo(req_));
        } else {
            http::response<http::string_body> res{http::status::not_found, req_.version()};
            res.set(http::field::content_type, "application/json");
            res.keep_alive(req_.keep_alive());
            res.body() = R"({"error":"not found"})";
            res.prepare_payload();
            sendResponse(std::move(res));
        }
    }

    void sendResponse(http::response<http::string_body> res) {
        auto sp = std::make_shared<http::response<http::string_body>>(std::move(res));
        http::async_write(socket_, *sp,
                          [self = shared_from_this(), sp](beast::error_code ec, std::size_t) {
                              if (!ec && sp->keep_alive()) {
                                  self->doRead();
                              } else {
                                  self->socket_.shutdown(tcp::socket::shutdown_send, ec);
                              }
                          });
    }

    tcp::socket socket_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
};

class Server {
  public:
    Server(asio::io_context& ioc, unsigned short port)
        : acceptor_(ioc, {asio::ip::make_address("0.0.0.0"), port}) {
        doAccept();
    }

  private:
    void doAccept() {
        acceptor_.async_accept([this](beast::error_code ec, tcp::socket socket) {
            if (!ec)
                std::make_shared<Session>(std::move(socket))->run();
            doAccept();
        });
    }
    tcp::acceptor acceptor_;
};

int main() {
    const unsigned short port = 8080;
    const int threads = static_cast<int>(std::thread::hardware_concurrency());

    asio::io_context ioc{threads};
    Server server{ioc, port};

    std::cout << "Server started on http://0.0.0.0:" << port << "\n";
    std::cout << "GET /personal/v1/info\n";

    std::vector<std::thread> pool;
    pool.reserve(threads - 1);
    for (int i = 0; i < threads - 1; ++i)
        pool.emplace_back([&ioc] { ioc.run(); });
    ioc.run();

    for (auto& t : pool)
        t.join();
    return 0;
}
