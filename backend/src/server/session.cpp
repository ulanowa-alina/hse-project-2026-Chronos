#include "session.hpp"

#include "personal/v1/info.hpp"

#include <boost/url.hpp>
#include <spdlog/spdlog.h>

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
                             spdlog::info("Connection closed by client {}",
                                          self->getClientAddress());
                             beast::error_code shutdownErr;
                             self->socket_.shutdown(tcp::socket::shutdown_send, shutdownErr);
                             return;
                         }
                         if (err) {
                             spdlog::error("Failed to read request from {}: {}",
                                           self->getClientAddress(), err.message());
                             return;
                         }

                         self->request_started_at_ = std::chrono::steady_clock::now();
                         self->handleRequest();
                     });
}

void Session::handleRequest() {
    const auto url_view_result = boost::urls::parse_origin_form(req_.target());
    const std::string route =
        url_view_result ? std::string(url_view_result->encoded_path()) : std::string(req_.target());

    spdlog::info("HTTP request: {} {} from {}", req_.method_string(), req_.target(),
                 getClientAddress());

    auto it = router_.find(route);
    if (it != router_.end()) {
        sendResponse(it->second(req_));
    } else {
        spdlog::error("Route not found: {} {}", req_.method_string(), req_.target());
        http::response<http::string_body> res{http::status::not_found, req_.version()};
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req_.keep_alive());
        res.body() = R"({"error":{"code":"BOARD_NOT_FOUND","message":"Resource not found"}})";
        res.prepare_payload();
        sendResponse(std::move(res));
    }
}

void Session::sendResponse(http::response<http::string_body> res) {

    const std::string method = std::string(req_.method_string());
    const std::string target = std::string(req_.target());
    const std::string client = getClientAddress();
    const unsigned response_status = res.result_int();
    const std::size_t response_size = res.body().size();

    auto response = std::make_shared<http::response<http::string_body>>(std::move(res));
    http::async_write(
        socket_, *response,
        [self = shared_from_this(), response, method, target, client, response_status,
         response_size](beast::error_code err, std::size_t) {
            const auto elapsed_time =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - self->request_started_at_)
                    .count();

            if (!err) {
                spdlog::info("HTTP response: {} {} -> {} in {} ms ({} bytes) for {}", method,
                             target, response_status, elapsed_time, response_size, client);
            }

            if (!err && response->keep_alive()) {

                self->doRead();
            } else {

                if (err) {
                    spdlog::error("Failed to write response to {}: {}", client, err.message());
                }

                beast::error_code shutdownErr;
                self->socket_.shutdown(tcp::socket::shutdown_send, shutdownErr);
                if (shutdownErr) {
                    spdlog::error("Socket shutdown error for {}: {}", client,
                                  shutdownErr.message());
                }
            }
        });
}

std::string Session::getClientAddress() const {
    beast::error_code err;
    const auto endpoint = socket_.remote_endpoint(err);

    if (err) {
        return "Not found";
    }

    return endpoint.address().to_string();
}
