#include "server.hpp"

#include "personal/v1/info.hpp"

#include <memory>

Server::Server(asio::io_context& ioc, const std::string& host, unsigned short port, ConnectionPool& pool)
    : acceptor_(ioc, {asio::ip::make_address(host), port})
    , pool_(pool) {
    router_["/personal/v1/info"] = [this](const http::request<http::string_body>& req) {
        return personal::v1::handleInfo(req, pool_);
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
