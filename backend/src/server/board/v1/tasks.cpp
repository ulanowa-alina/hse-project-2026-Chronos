#include "tasks.hpp"

#include "../../../../repositories/board_repository.hpp"
#include "../../../../repositories/task_repository.hpp"

#include <array>
#include <boost/url.hpp>
#include <ctime>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

namespace board::v1 {

namespace {

std::string time_to_string_iso8601(std::time_t t) {
    std::array<char, 25> buffer{};
    if (std::strftime(buffer.data(), buffer.size(), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t))) {
        return {buffer.data()};
    }
    return "";
}

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

auto build_error_response(const http::request<http::string_body>& req, http::status status,
                          const std::string& code, const std::string& message,
                          const json& details = json()) -> http::response<http::string_body> {
    json error = {
        {"code", code},
        {"message", message},
    };
    if (!details.is_null() && !details.empty()) {
        error["details"] = details;
    }
    return build_json_response(req, status, json{{"error", error}});
}

int parse_board_id(const http::request<http::string_body>& req) {
    const auto url_view_result = boost::urls::parse_origin_form(req.target());
    if (!url_view_result) {
        throw std::invalid_argument("request target is invalid");
    }

    const auto params = url_view_result->params();
    const auto board_id_param = params.find("board_id");
    if (board_id_param == params.end()) {
        throw std::invalid_argument("board_id is required");
    }

    int board_id = 0;
    try {
        std::size_t parsed_chars = 0;
        const std::string board_id_value((*board_id_param).value);
        board_id = std::stoi(board_id_value, &parsed_chars);
        if (parsed_chars != board_id_value.size()) {
            throw std::invalid_argument("board_id must be an integer");
        }
    } catch (const std::exception&) {
        throw std::invalid_argument("board_id must be an integer");
    }
    if (board_id <= 0) {
        throw std::invalid_argument("board_id must be positive");
    }

    return board_id;
}

json model_to_json(const Task& task) {
    json data = {
        {"id", task.id_},
        {"board_id", task.board_id_},
        {"title", task.title_},
        {"description", task.description_},
        {"status_id", task.status_id_},
        {"priority_color", task.priority_color_},
        {"created_at", time_to_string_iso8601(task.created_at_)},
        {"updated_at", time_to_string_iso8601(task.updated_at_)},
    };

    if (task.deadline_) {
        data["deadline"] = time_to_string_iso8601(*task.deadline_);
    } else {
        data["deadline"] = nullptr;
    }

    return data;
}

} // namespace

auto handleTasks(const http::request<http::string_body>& req,
                 ConnectionPool& pool) -> http::response<http::string_body> {
    if (req.method() != http::verb::get) {
        return build_error_response(req, http::status::method_not_allowed, "METHOD_NOT_ALLOWED",
                                    "Only GET is supported for this endpoint");
    }

    try {
        const int board_id = parse_board_id(req);

        BoardRepository board_repository(pool);
        if (!board_repository.find_by_id(board_id)) {
            return build_error_response(req, http::status::not_found, "BOARD_NOT_FOUND",
                                        "Board not found");
        }

        TaskRepository task_repository(pool);
        const auto tasks = task_repository.find_by_board_id(board_id);

        json data = json::array();
        for (const auto& task : tasks) {
            data.push_back(model_to_json(task));
        }

        return build_json_response(req, http::status::ok, json{{"data", data}});
    } catch (const std::invalid_argument& e) {
        const std::string message = e.what();
        std::string error_code = "INVALID_FORMAT";
        if (message == "board_id is required") {
            error_code = "MISSING_FIELD";
        } else if (message == "board_id must be positive") {
            error_code = "VALIDATION_ERROR";
        }
        return build_error_response(req, http::status::bad_request, error_code, message,
                                    json{{"board_id", message}});
    } catch (const std::runtime_error& e) {
        return build_error_response(req, http::status::internal_server_error, "DATABASE_ERROR",
                                    e.what());
    } catch (const std::exception& e) {
        return build_error_response(req, http::status::internal_server_error, "INTERNAL_ERROR",
                                    e.what());
    }
}

} // namespace board::v1
