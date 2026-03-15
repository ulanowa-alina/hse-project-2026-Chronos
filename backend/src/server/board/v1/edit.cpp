#include "edit.hpp"

#include "board.hpp"
#include "board_repository.hpp"

#include <boost/json.hpp>

namespace json = boost::json;
namespace http = boost::beast::http;

namespace board::v1 {

http::response<http::string_body> handleEdit(const http::request<http::string_body>& req,
                                             BoardRepository& repo) {

    if (req.method() != http::verb::patch) {
        http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::access_control_allow_origin, "*");
        res.keep_alive(req.keep_alive());
        res.body() = R"({"error":"method_not_allowed"})";
        res.prepare_payload();
        return res;
    }

    json::object body;
    try {
        body = json::parse(req.body()).as_object();
    } catch (...) {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::access_control_allow_origin, "*");
        res.keep_alive(req.keep_alive());
        res.body() = R"({"error":"invalid_json"})";
        res.prepare_payload();
        return res;
    }

    if (!body.contains("board_id") || !body.contains("title")) {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::access_control_allow_origin, "*");
        res.keep_alive(req.keep_alive());
        res.body() = R"({"error":"board_id and title are required"})";
        res.prepare_payload();
        return res;
    }

    int board_id = static_cast<int>(body.at("board_id").as_int64());
    std::string new_title = std::string(body.at("title").as_string());

    auto board_opt = repo.find_by_id(board_id);
    if (!board_opt.has_value()) {
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::access_control_allow_origin, "*");
        res.keep_alive(req.keep_alive());
        res.body() = R"({"error":"board_not_found"})";
        res.prepare_payload();
        return res;
    }

    Board board = board_opt.value();
    board.title_ = new_title;
    Board updated = repo.save(board);

    json::object response_body;
    response_body["id"] = updated.id_;
    response_body["user_id"] = updated.user_id_;
    response_body["title"] = updated.title_;
    response_body["description"] = updated.description_;
    response_body["is_private"] = updated.is_private_;

    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::access_control_allow_origin, "*");
    res.keep_alive(req.keep_alive());
    res.body() = json::serialize(response_body);
    res.prepare_payload();
    return res;
}

} // namespace board::v1
