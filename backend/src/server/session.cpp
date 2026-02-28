#include "session.hpp"

#include "personal/v1/info.hpp"

#include <iostream>

Session::Session(tcp::socket socket, Router router)
    : socket_(std::move(socket))
    , router_(std::move(router)) {
}

void Session::run() {
    doRead();
}

void Session::doRead() {
    http::async_read(socket_, buffer_, req_ = {},
                     [self = shared_from_this()](beast::error_code err, std::size_t) {
                         if (err == http::error::end_of_stream) {
                             beast::error_code shutdownErr;
                             self->socket_.shutdown(tcp::socket::shutdown_send, shutdownErr);
                             return;
                         }
                         if (!err) {
                             self->handleRequest();
                         }
                     });
}

void Session::handleRequest() {
    const std::string target{req_.target()};
    auto it = router_.find(target);
    if (it != router_.end()) {
        sendResponse(it->second(req_));
    } else {
        http::response<http::string_body> res{http::status::not_found, req_.version()};
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req_.keep_alive());
        res.body() = R"({"error":"not found"})";
        res.prepare_payload();
        sendResponse(std::move(res));
    }
}

void Session::sendResponse(http::response<http::string_body> res) {
    auto response = std::make_shared<http::response<http::string_body>>(std::move(res));
    http::async_write(socket_, *response,
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
