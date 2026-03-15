#include "create_user.hpp"

#include "db/users_repo.hpp"

#include <nlohmann/json.hpp>

namespace users {

namespace {

using nlohmann::json;

auto build_json_response(const http::request<http::string_body>& req, http::status status,
                         const json& body) -> http::response<http::string_body> {
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::access_control_allow_origin, "*");
    res.keep_alive(req.keep_alive());
    res.body() = body.dump();
    res.prepare_payload();
    return res;
}

auto build_bad_request(const http::request<http::string_body>& req,
                       const std::string& msg) -> http::response<http::string_body> {
    return build_json_response(req, http::status::bad_request, json{{"error", msg}});
}

auto build_db_error(const http::request<http::string_body>& req,
                    const std::string& msg) -> http::response<http::string_body> {
    return build_json_response(req, http::status::internal_server_error,
                               json{{"error", "db_error"}, {"details", msg}});
}

auto build_create_response(const http::request<http::string_body>& req,
                           const User& created) -> http::response<http::string_body> {
    json out;
    out["id"] = created.id;
    out["email"] = created.email;
    out["name"] = created.name;
    out["created_at"] = created.created_at;
    return build_json_response(req, http::status::created, out);
}

json parse_body(const http::request<http::string_body>& req) {
    return json::parse(req.body());
}

NewUser parse_new_user(const json& body) {
    auto has_str = [&](const char* key) {
        return body.contains(key) && body[key].is_string() && !body[key].get<std::string>().empty();
    };

    if (!has_str("email") || !has_str("name") || !has_str("password")) {
        throw std::invalid_argument("missing_fields");
    }

    NewUser nu;
    nu.email = body["email"].get<std::string>();
    nu.name = body["name"].get<std::string>();
    nu.password_hash = body["password"].get<std::string>();
    return nu;
}

User createUser(ConnectionPool& pool, const NewUser& nu) {
    return insert_user(pool, nu);
}

} // namespace

auto handleCreate(const http::request<http::string_body>& req,
                  ConnectionPool& pool) -> http::response<http::string_body> {
    try {
        const json body = parse_body(req);
        const NewUser nu = parse_new_user(body);
        const User created = createUser(pool, nu);
        return build_create_response(req, created);
    } catch (const nlohmann::json::exception&) {
        return build_bad_request(req, "bad_json");
    } catch (const std::invalid_argument& e) {
        (void) e;
        return build_bad_request(req, R"({"missing_fields":["email","name","password"]})");
    } catch (const std::exception& e) {
        return build_db_error(req, e.what());
    }
}

} // namespace users