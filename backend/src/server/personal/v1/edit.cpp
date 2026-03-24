#include "edit.hpp"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include <pqxx/pqxx>
#include <string>

namespace http = boost::beast::http;

namespace personal::v1 {

auto handleEdit(const http::request<http::string_body>& req, ConnectionPool& pool)
    -> http::response<http::string_body> {
    (void) pool;

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

    if (!body.contains("id") || !body["id"].is_number_integer()) {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::access_control_allow_origin, "*");
        res.keep_alive(req.keep_alive());
        res.body() = R"({"error":"missing_or_bad_id"})";
        res.prepare_payload();
        return res;
    }

    const bool has_email = body.contains("email") && body["email"].is_string();
    const bool has_name = body.contains("name") && body["name"].is_string();
    const bool has_password = body.contains("password") && body["password"].is_string();

    if (!has_email && !has_name && !has_password) {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::access_control_allow_origin, "*");
        res.keep_alive(req.keep_alive());
        res.body() = R"({"error":"no_fields_to_update"})";
        res.prepare_payload();
        return res;
    }

    const int id = body["id"].get<int>();

    const std::string email = has_email ? body["email"].get<std::string>() : "";
    const std::string name = has_name ? body["name"].get<std::string>() : "";
    const std::string password = has_password ? body["password"].get<std::string>() : "";

    const std::string password_hash = has_password ? ("hash:" + password) : "";

    try {
        auto h = pool.acquire();
        pqxx::work tx(h.conn());

        std::string sql = "UPDATE users SET ";
        std::vector<std::string> sets;
        std::vector<std::string> params;

        if (has_email) {
            sets.push_back("email = $" + std::to_string(params.size() + 1));
            params.push_back(email);
        }
        if (has_name) {
            sets.push_back("name = $" + std::to_string(params.size() + 1));
            params.push_back(name);
        }
        if (has_password) {
            sets.push_back("password_hash = $" + std::to_string(params.size() + 1));
            params.push_back(password_hash);
        }

        for (std::size_t i = 0; i < sets.size(); ++i) {
            if (i)
                sql += ", ";
            sql += sets[i];
        }

        sql += " WHERE id = $" + std::to_string(params.size() + 1);
        params.push_back(std::to_string(id));

        sql += " RETURNING id, email, name, created_at";

        pqxx::params p;
        for (const auto& s : params)
            p.append(s);

        pqxx::result r = tx.exec_params(sql, p);
        tx.commit();

        if (r.empty()) {
            http::response<http::string_body> res{http::status::not_found, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::access_control_allow_origin, "*");
            res.keep_alive(req.keep_alive());
            res.body() = R"({"error":"user_not_found"})";
            res.prepare_payload();
            return res;
        }

        nlohmann::json out;
        out["id"] = r[0][0].as<int>();
        out["email"] = r[0][1].as<std::string>();
        out["name"] = r[0][2].as<std::string>();
        out["created_at"] = std::string(r[0][3].c_str());

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::access_control_allow_origin, "*");
        res.keep_alive(req.keep_alive());
        res.body() = out.dump();
        res.prepare_payload();
        return res;

    } catch (const pqxx::sql_error& e) {
        http::response<http::string_body> res{http::status::conflict, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::access_control_allow_origin, "*");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(R"({"error":"conflict","details":")") + e.what() + R"("})";
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

} // namespace personal::v1
