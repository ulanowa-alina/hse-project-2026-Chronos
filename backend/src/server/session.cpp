#include "session.hpp"

#include "metrics.hpp"
#include "personal/v1/info.hpp"

#include <boost/url.hpp>
#include <chrono>
#include <iostream>
#include <string>

namespace {

bool is_error_response(const Response& response) {
    return response.result_int() >= 400;
}

std::string method_to_string(http::verb method) {
    return std::string(http::to_string(method));
}

std::string model_from_route(const std::string& route) {
    if (route.rfind("/board/", 0) == 0) {
        return "board";
    }
    if (route.rfind("/status/", 0) == 0) {
        return "status";
    }
    if (route.rfind("/task/", 0) == 0) {
        return "task";
    }
    if (route.rfind("/auth/", 0) == 0 || route.rfind("/personal/", 0) == 0) {
        return "user";
    }

    return "unknown";
}

std::string operation_from_route(const std::string& route) {
    const std::size_t last_slash = route.find_last_of('/');
    if (last_slash == std::string::npos || last_slash + 1 >= route.size()) {
        return "unknown";
    }

    return route.substr(last_slash + 1);
}

std::string error_code_from_response(const Response& response) {
    const std::string& body = response.body();
    const std::string code_key = R"("code")";
    const std::size_t code_pos = body.find(code_key);
    if (code_pos == std::string::npos) {
        return "unknown";
    }

    const std::size_t colon_pos = body.find(':', code_pos + code_key.size());
    if (colon_pos == std::string::npos) {
        return "unknown";
    }

    const std::size_t value_start = body.find('"', colon_pos + 1);
    if (value_start == std::string::npos) {
        return "unknown";
    }

    const std::size_t value_end = body.find('"', value_start + 1);
    if (value_end == std::string::npos) {
        return "unknown";
    }

    return body.substr(value_start + 1, value_end - value_start - 1);
}

void record_response_metrics(const Response& response, const std::string& route,
                             const std::string& method, double duration_seconds) {
    const std::string model = model_from_route(route);
    const std::string operation = model == "unknown" ? "unknown" : operation_from_route(route);
    const int status_code = response.result_int();

    server::metrics::record_api_request(model, operation, method, status_code);
    server::metrics::observe_api_request_duration(model, operation, method, status_code,
                                                  duration_seconds);

    if (is_error_response(response)) {
        const std::string error_code = error_code_from_response(response);

        server::metrics::record_http_error();
        server::metrics::record_api_error(model, operation, error_code, status_code);
        if (error_code == "DATABASE_ERROR") {
            server::metrics::record_db_error();
        }
    }
}

} // namespace

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
    server::metrics::record_http_request();
    const auto started_at = std::chrono::steady_clock::now();

    const auto url_view_result = boost::urls::parse_origin_form(req_.target());
    const std::string route =
        url_view_result ? std::string(url_view_result->encoded_path()) : std::string(req_.target());
    const std::string method = method_to_string(req_.method());

    auto it = router_.find(route);
    if (it != router_.end()) {
        Response response = it->second(req_);
        const std::chrono::duration<double> duration =
            std::chrono::steady_clock::now() - started_at;
        record_response_metrics(response, route, method, duration.count());
        sendResponse(std::move(response));
    } else {
        http::response<http::string_body> res{http::status::not_found, req_.version()};
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req_.keep_alive());
        res.body() = R"({"error":{"code":"BOARD_NOT_FOUND","message":"Resource not found"}})";
        res.prepare_payload();
        const std::chrono::duration<double> duration =
            std::chrono::steady_clock::now() - started_at;
        record_response_metrics(res, route, method, duration.count());
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
