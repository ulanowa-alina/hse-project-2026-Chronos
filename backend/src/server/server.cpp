#include "server.hpp"

#include "personal/v1/info.hpp"

#include <memory>

Server::Server(asio::io_context& ioc, const std::string& host, unsigned short port)
    : acceptor_(ioc, {asio::ip::make_address(host), port}) {
    router_["/personal/v1/info"] = personal::v1::handleInfo;
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
