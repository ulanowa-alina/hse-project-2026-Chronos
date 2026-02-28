#include "server.hpp"

#include "session.hpp"

#include <memory>

Server::Server(asio::io_context& ioc, unsigned short port)
    : acceptor_(ioc, {asio::ip::make_address("0.0.0.0"), port}) {
    doAccept();
}

void Server::doAccept() {
    acceptor_.async_accept([this](beast::error_code err, tcp::socket socket) {
        if (!err) {
            std::make_shared<Session>(std::move(socket))->run();
        }
        doAccept();
    });
}
