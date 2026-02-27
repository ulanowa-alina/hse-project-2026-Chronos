#include "server/personal/v1/info.hpp"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

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
                         [self = shared_from_this()](beast::error_code err, std::size_t) {
                             if (!err) {
                                 self->handleRequest();
                             }
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
        auto response = std::make_shared<http::response<http::string_body>>(std::move(res));
        http::async_write(
            socket_, *response,
            [self = shared_from_this(), response](beast::error_code err, std::size_t) {
                if (!err && response->keep_alive()) {
                    self->doRead();
                } else {
                    beast::error_code shutdownErr;
                    self->socket_.shutdown(tcp::socket::shutdown_send, shutdownErr);
                    if (shutdownErr) {
                        std::cerr << "Shutdown error: " << shutdownErr.message() << "\n";
                    }
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
        acceptor_.async_accept([this](beast::error_code err, tcp::socket socket) {
            if (!err) {
                std::make_shared<Session>(std::move(socket))->run();
            }
            doAccept();
        });
    }

    tcp::acceptor acceptor_;
};

auto main() -> int {
    try {
        const unsigned short port = 8080;
        const int threadCount = static_cast<int>(std::thread::hardware_concurrency());

        asio::io_context ioc{threadCount};
        Server server{ioc, port};

        std::cout << "Server started on http://0.0.0.0:" << port << "\n";
        std::cout << "GET /personal/v1/info\n";

        std::vector<std::thread> pool;
        pool.reserve(static_cast<std::size_t>(threadCount - 1));
        for (int idx = 0; idx < threadCount - 1; ++idx) {
            pool.emplace_back([&ioc] { ioc.run(); });
        }
        ioc.run();

        for (auto& thread : pool) {
            thread.join();
        }
    } catch (const std::exception& exc) {
        std::cerr << "Fatal error: " << exc.what() << "\n";
        return 1;
    }
    return 0;
}