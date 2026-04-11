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
    throw std::runtime_error("Failed to format timestamp");
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

int require_int_field(const http::request<http::string_body>& req, const std::string& key) {
    const auto url_view_result = boost::urls::parse_origin_form(req.target());
    if (!url_view_result) {
        throw std::invalid_argument("type:" + key);
    }

    const auto params = url_view_result->params();
    const auto param = params.find(key);
    if (param == params.end()) {
        throw std::invalid_argument("missing:" + key);
    }

    try {
        std::size_t parsed_chars = 0;
        const std::string value((*param).value);
        const auto parsed = std::stoll(value, &parsed_chars);
        if (parsed_chars != value.size()) {
            throw std::invalid_argument("type:" + key);
        }
        if (parsed <= 0 || parsed > std::numeric_limits<int>::max()) {
            throw std::invalid_argument("value:" + key);
        }

        return static_cast<int>(parsed);
    } catch (const std::invalid_argument&) {
        throw;
    } catch (const std::exception&) {
        throw std::invalid_argument("type:" + key);
    }
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
        const int board_id = require_int_field(req, "board_id");

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
        if (message.rfind("missing:", 0) == 0) {
            const std::string field = message.substr(8);
            return build_error_response(req, http::status::bad_request, "MISSING_FIELD",
                                        "Missing required field",
                                        json{{field, "Field " + field + " is required"}});
        }
        if (message.rfind("type:", 0) == 0) {
            const std::string field = message.substr(5);
            return build_error_response(req, http::status::bad_request, "INVALID_FORMAT",
                                        "Invalid field format",
                                        json{{field, "Field " + field + " has invalid type"}});
        }
        if (message.rfind("value:", 0) == 0) {
            const std::string field = message.substr(6);
            return build_error_response(
                req, http::status::bad_request, "VALIDATION_ERROR", "Invalid field value",
                json{{field, "Field " + field + " must be a positive integer"}});
        }
        return build_error_response(req, http::status::bad_request, "VALIDATION_ERROR", message);
    } catch (const std::runtime_error& e) {
        return build_error_response(req, http::status::internal_server_error, "DATABASE_ERROR",
                                    e.what());
    } catch (const std::exception& e) {
        return build_error_response(req, http::status::internal_server_error, "INTERNAL_ERROR",
                                    e.what());
    }
}

} // namespace board::v1
