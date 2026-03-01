#include "server.hpp"

#include "personal/v1/info.hpp"
#include "users/create_user.hpp"

#include <memory>

Server::Server(asio::io_context& ioc, const std::string& host, unsigned short port,
               ConnectionPool& pool)
    : acceptor_(ioc, {asio::ip::make_address(host), port})
    , pool_(pool) {
    router_["/personal/v1/info"] = [this](const http::request<http::string_body>& req) {
        return personal::v1::handleInfo(req, pool_);
    };
    router_["/users"] = [this](const http::request<http::string_body>& req) {
        if (req.method() == http::verb::post) {
            return users::handleCreate(req, pool_);
        }
        http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::access_control_allow_origin, "*");
        res.keep_alive(req.keep_alive());
        res.body() = R"({"error":"method_not_allowed"})";
        res.prepare_payload();
        return res;
    };
    doAccept();
}

void Server::doAccept() {
    acceptor_.async_accept([this](beast::error_code err, tcp::socket socket) {
        if (!err) {
            std::make_shared<Session>(std::move(socket), router_)->run();
        }
        doAccept();
    });
}
