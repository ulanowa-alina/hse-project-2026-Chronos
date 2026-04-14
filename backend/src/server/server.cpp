#include "server.hpp"

#include "auth/v1/login.hpp"
#include "auth/v1/register.hpp"
#include "auth/with_auth.hpp"
#include "board/v1/tasks.hpp"
#include "personal/v1/edit.hpp"
#include "personal/v1/info.hpp"
#include "status/v1/create.hpp"
#include "task/v1/create.hpp"
#include "task/v1/delete.hpp"

#include <memory>

Server::Server(asio::io_context& ioc, const std::string& host, unsigned short port,
               ConnectionPool& pool)
    : acceptor_(ioc, {asio::ip::make_address(host), port})
    , pool_(pool) {
    router_["/personal/v1/info"] =
        auth::with_auth([this](const http::request<http::string_body>& req, int user_id) {
            if (req.method() == http::verb::get) {
                return personal::v1::handleInfo(req, pool_, user_id);
            }

            http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::access_control_allow_origin, "*");
            res.keep_alive(req.keep_alive());
            res.body() = R"({"error":{"code":"DUPLICATE_RESOURCE","message":"Method not allowed"}})";
            res.prepare_payload();
            return res;
        });

    router_["/personal/v1/edit"] =
        auth::with_auth([this](const http::request<http::string_body>& req, int user_id) {
            if (req.method() == http::verb::put) {
                return personal::v1::handleEdit(req, pool_, user_id);
            }

            http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::access_control_allow_origin, "*");
            res.keep_alive(req.keep_alive());
            res.body() = R"({"error":{"code":"DUPLICATE_RESOURCE","message":"Method not allowed"}})";
            res.prepare_payload();
            return res;
        });

    router_["/task/v1/delete"] =
        auth::with_auth([this](const http::request<http::string_body>& req, int user_id) {
            if (req.method() == http::verb::delete_) {
                return task::v1::handleDelete(req, pool_, user_id);
            }

            http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::access_control_allow_origin, "*");
            res.keep_alive(req.keep_alive());
            res.body() =
                R"({"error":{"code":"DUPLICATE_RESOURCE","message":"Method not allowed"}})";
            res.prepare_payload();
            return res;
        });

    router_["/auth/v1/login"] = [this](const http::request<http::string_body>& req) {
        if (req.method() == http::verb::post) {
            return auth::v1::handleLogin(req, pool_);
        }

        http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::access_control_allow_origin, "*");
        res.keep_alive(req.keep_alive());
        res.body() = R"({"error":{"code":"DUPLICATE_RESOURCE","message":"Method not allowed"}})";
        res.prepare_payload();
        return res;
    };

    router_["/auth/v1/register"] = [this](const http::request<http::string_body>& req) {
        if (req.method() == http::verb::post) {
            return auth::v1::handleRegister(req, pool_);
        }

        http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::access_control_allow_origin, "*");
        res.keep_alive(req.keep_alive());
        res.body() = R"({"error":{"code":"DUPLICATE_RESOURCE","message":"Method not allowed"}})";
        res.prepare_payload();
        return res;
    };

    router_["/board/v1/tasks"] = [this](const http::request<http::string_body>& req) {
        if (req.method() == http::verb::get) {
            return board::v1::handleTasks(req, pool_);
        }

        http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::access_control_allow_origin, "*");
        res.keep_alive(req.keep_alive());
        res.body() = R"({"error":{"code":"DUPLICATE_RESOURCE","message":"Method not allowed"}})";
        res.prepare_payload();
        return res;
    };

    router_["/task/v1/create"] = [this](const http::request<http::string_body>& req) {
        if (req.method() == http::verb::post) {
            return task::v1::handleCreate(req, pool_);
        }

        http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::access_control_allow_origin, "*");
        res.keep_alive(req.keep_alive());
        res.body() = R"({"error":{"code":"DUPLICATE_RESOURCE","message":"Method not allowed"}})";
        res.prepare_payload();
        return res;
    };

    router_["/status/v1/create"] =
        auth::with_auth([this](const http::request<http::string_body>& req, int user_id) {
            if (req.method() == http::verb::post) {
                return status::v1::handleCreate(req, pool_, user_id);
            }

            http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::access_control_allow_origin, "*");
            res.keep_alive(req.keep_alive());
            res.body() =
                R"({"error":{"code":"DUPLICATE_RESOURCE","message":"Method not allowed"}})";
            res.prepare_payload();
            return res;
        });
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
