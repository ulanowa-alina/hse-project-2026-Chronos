#include "create.hpp"

#include "../../../../repositories/board_repository.hpp"
#include "../../../../repositories/status_repository.hpp"
#include "../../../../repositories/task_repository.hpp"

#include <array>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <limits>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>
#include <string>

using json = nlohmann::json;

namespace task::v1 {

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
    json error = {{"code", code}, {"message", message}};
    if (!details.is_null() && !details.empty()) {
        error["details"] = details;
    }
    return build_json_response(req, status, json{{"error", error}});
}

std::time_t parse_iso8601_utc(const std::string& value) {
    if (value.empty() || value.back() != 'Z') {
        throw std::invalid_argument("value:deadline");
    }

    std::tm tm = {};
    std::istringstream stream(value.substr(0, value.size() - 1));
    stream >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (stream.fail() || !stream.eof()) {
        throw std::invalid_argument("value:deadline");
    }

    const std::time_t parsed = timegm(&tm);
    if (parsed < 0) {
        throw std::invalid_argument("value:deadline");
    }

    const auto tp = std::chrono::system_clock::from_time_t(parsed);
    return std::chrono::system_clock::to_time_t(tp);
}

int require_int_field(const json& body, const std::string& key) {
    try {
        const int value = body.at(key).get<int>();
        if (value <= 0) {
            throw std::invalid_argument("value:" + key);
        }

        return value;
    } catch (const json::out_of_range&) {
        throw std::invalid_argument("missing:" + key);
    } catch (const json::type_error&) {
        throw std::invalid_argument("type:" + key);
    }
}

std::string require_string_field(const json& body, const std::string& key) {
    try {
        return body.at(key).get<std::string>();
    } catch (const json::out_of_range&) {
        throw std::invalid_argument("missing:" + key);
    } catch (const json::type_error&) {
        throw std::invalid_argument("type:" + key);
    }
}

std::string optional_string_field(const json& body, const std::string& key) {
    try {
        if (body.at(key).is_null()) {
            return "";
        }

        return body.at(key).get<std::string>();
    } catch (const json::out_of_range&) {
        return "";
    } catch (const json::type_error&) {
        throw std::invalid_argument("type:" + key);
    }
}

std::optional<std::time_t> optional_deadline_field(const json& body) {
    try {
        if (body.at("deadline").is_null()) {
            return std::nullopt;
        }

        return parse_iso8601_utc(body.at("deadline").get<std::string>());
    } catch (const json::out_of_range&) {
        return std::nullopt;
    } catch (const json::type_error&) {
        throw std::invalid_argument("type:deadline");
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

auto handleCreate(const http::request<http::string_body>& req,
                  ConnectionPool& pool) -> http::response<http::string_body> {
    if (req.method() != http::verb::post) {
        return build_error_response(req, http::status::method_not_allowed, "METHOD_NOT_ALLOWED",
                                    "Only POST is supported for this endpoint");
    }

    json body;
    try {
        body = json::parse(req.body());
    } catch (const json::exception&) {
        return build_error_response(req, http::status::bad_request, "INVALID_FORMAT",
                                    "Request body contains invalid JSON");
    }

    if (!body.is_object()) {
        return build_error_response(req, http::status::bad_request, "INVALID_FORMAT",
                                    "Request body must be a JSON object");
    }

    try {
        const int board_id = require_int_field(body, "board_id");
        const std::string title = require_string_field(body, "title");
        const std::string description = optional_string_field(body, "description");
        const int status_id = require_int_field(body, "status_id");
        const std::string priority_color = require_string_field(body, "priority_color");
        const std::optional<std::time_t> deadline = optional_deadline_field(body);

        if (title.empty() || title.size() > 100) {
            return build_error_response(
                req, http::status::bad_request, "VALIDATION_ERROR", "Invalid field value",
                json{{"title", "Title must be between 1 and 100 characters"}});
        }
        if (description.size() > 1000) {
            return build_error_response(
                req, http::status::bad_request, "VALIDATION_ERROR", "Invalid field value",
                json{{"description", "Description cannot exceed 1000 characters"}});
        }
        if (priority_color.empty() || priority_color.size() > 50) {
            return build_error_response(
                req, http::status::bad_request, "VALIDATION_ERROR", "Invalid field value",
                json{{"priority_color", "Priority color must be between 1 and 50 characters"}});
        }

        BoardRepository board_repository(pool);
        if (!board_repository.find_by_id(board_id).has_value()) {
            return build_error_response(req, http::status::not_found, "BOARD_NOT_FOUND",
                                        "Board not found");
        }

        StatusRepository status_repository(pool);
        const auto status = status_repository.find_by_id(status_id);
        if (!status.has_value() || status->board_id_ != board_id) {
            return build_error_response(
                req, http::status::bad_request, "VALIDATION_ERROR", "Invalid field value",
                json{{"status_id", "status_id must reference a status from this board"}});
        }

        TaskRepository task_repository(pool);
        const Task new_task(0, board_id, title, description, deadline, status_id, priority_color, 0,
                            0);
        const Task created = task_repository.save(new_task);

        return build_json_response(req, http::status::ok, json{{"data", model_to_json(created)}});
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
            const std::string detail = field == "deadline"
                                           ? "Field deadline must be a valid ISO 8601 UTC datetime"
                                           : "Field " + field + " must be a positive integer";
            return build_error_response(req, http::status::bad_request, "VALIDATION_ERROR",
                                        "Invalid field value", json{{field, detail}});
        }
        return build_error_response(req, http::status::bad_request, "VALIDATION_ERROR", e.what());
    } catch (const std::runtime_error& e) {
        return build_error_response(req, http::status::internal_server_error, "DATABASE_ERROR",
                                    e.what());
    } catch (const std::exception& e) {
        return build_error_response(req, http::status::internal_server_error, "INTERNAL_ERROR",
                                    e.what());
    }
}

} // namespace task::v1
