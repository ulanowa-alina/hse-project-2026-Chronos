#include "create_user.hpp"

#include "db/users_repo.hpp"
#include <nlohmann/json.hpp>

namespace users {

auto handleCreate(const http::request<http::string_body>& req, ConnectionPool& pool)
    -> http::response<http::string_body> {

    using nlohmann::json;

    json body;
    try {
        body = json::parse(req.body());
    } catch (...) {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::access_control_allow_origin, "*");
        res.keep_alive(req.keep_alive());
        res.body() = R"({"error":"bad_json"})";
        res.prepare_payload();
        return res;
    }

    if (!body.contains("email") || !body["email"].is_string() ||
        !body.contains("name") || !body["name"].is_string() ||
        !body.contains("password") || !body["password"].is_string()) {

        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::access_control_allow_origin, "*");
        res.keep_alive(req.keep_alive());
        res.body() = R"({"error":"missing_fields","need":["email","name","password"]})";
        res.prepare_payload();
        return res;
    }

    NewUser nu;
    nu.email = body["email"].get<std::string>();
    nu.name = body["name"].get<std::string>();

    nu.password_hash = body["password"].get<std::string>();

    try {
        User created = insert_user(pool, nu);

        http::response<http::string_body> res{http::status::created, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::access_control_allow_origin, "*");
        res.keep_alive(req.keep_alive());

        json out;
        out["id"] = created.id;
        out["email"] = created.email;
        out["name"] = created.name;
        out["created_at"] = created.created_at;

        res.body() = out.dump();
        res.prepare_payload();
        return res;

    } catch (const std::exception& e) {
        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::access_control_allow_origin, "*");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(R"({"error":"db_error","details":")") + e.what() + R"("})";
        res.prepare_payload();
        return res;
    }
}

} // namespace users