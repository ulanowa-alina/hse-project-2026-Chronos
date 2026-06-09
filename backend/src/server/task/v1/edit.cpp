#include "edit.hpp"

#include "../../../../repositories/board_repository.hpp"
#include "../../../../repositories/status_repository.hpp"
#include "../../../../repositories/task_repository.hpp"
#include "../../utils/response_utils.hpp"

#include <array>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>

using json = nlohmann::json;

namespace task::v1 {

namespace {

const size_t MAX_TITLE_SIZE = 100;
const size_t MAX_DESCRIPTION_SIZE = 1000;
const size_t MAX_PRIORITY_COLOR_SIZE = 500;

json collect_missing_fields(const json& body) {
    json details = json::object();

    if (!body.contains("task_id")) {
        details["task_id"] = "Missing required field";
    }
    if (!body.contains("title")) {
        details["title"] = "Missing required field";
    }
    if (!body.contains("description")) {
        details["description"] = "Missing required field";
    }
    if (!body.contains("status_id")) {
        details["status_id"] = "Missing required field";
    }
    if (!body.contains("priority_color")) {
        details["priority_color"] = "Missing required field";
    }

    return details;
}

int require_positive_int_field(const json& body, const std::string& key) {
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

std::string time_to_string_iso8601(std::time_t t) {
    std::array<char, 25> buffer{};
    if (std::strftime(buffer.data(), buffer.size(), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t))) {
        return {buffer.data()};
    }
    throw std::runtime_error("Failed to format timestamp");
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

json model_to_json(const Task& task) {
    json data = {{"id", task.id_},
                 {"board_id", task.board_id_},
                 {"title", task.title_},
                 {"description", task.description_},
                 {"status_id", task.status_id_},
                 {"priority_color", task.priority_color_},
                 {"is_completed", task.is_completed_},
                 {"created_at", time_to_string_iso8601(task.created_at_)},
                 {"updated_at", time_to_string_iso8601(task.updated_at_)}};

    if (task.deadline_) {
        data["deadline"] = time_to_string_iso8601(*task.deadline_);
    } else {
        data["deadline"] = nullptr;
    }

    return data;
}

} // namespace

auto handleEdit(const http::request<http::string_body>& req, ConnectionPool& pool,
                int user_id) -> http::response<http::string_body> {
    spdlog::info("Task edit request received");

    if (req.method() != http::verb::patch) {
        spdlog::error("Task edit rejected: method not allowed");
        return server::utils::build_error_response(req, http::status::method_not_allowed,
                                                   "DUPLICATE_RESOURCE", "Method not allowed");
    }

    json body;
    try {
        body = json::parse(req.body());
    } catch (const json::exception&) {
        spdlog::error("Task edit rejected: invalid JSON format");
        return server::utils::build_error_response(req, http::status::bad_request, "INVALID_FORMAT",
                                                   "Invalid JSON format");
    }

    if (!body.is_object()) {
        spdlog::error("Task edit rejected: invalid JSON format");
        return server::utils::build_error_response(req, http::status::bad_request, "INVALID_FORMAT",
                                                   "Invalid JSON format");
    }

    const json details = collect_missing_fields(body);
    if (!details.empty()) {
        spdlog::error("Task edit rejected: missing required fields");
        return server::utils::build_error_response(req, http::status::bad_request, "MISSING_FIELD",
                                                   "Missing required fields", details);
    }

    try {
        const int task_id = require_positive_int_field(body, "task_id");

        TaskRepository task_repository(pool);
        const std::optional<Task> existing_task = task_repository.find_by_id(task_id);
        if (!existing_task.has_value()) {
            spdlog::error("Task edit rejected: task with id={} not found", task_id);
            return server::utils::build_error_response(req, http::status::not_found,
                                                       "TASK_NOT_FOUND", "Task not found");
        }

        BoardRepository board_repository(pool);
        const std::optional<Board> task_board =
            board_repository.find_by_id(existing_task->board_id_);
        if (!task_board.has_value()) {
            spdlog::error("Task edit rejected: board with id={} not found",
                          existing_task->board_id_);
            return server::utils::build_error_response(req, http::status::not_found,
                                                       "BOARD_NOT_FOUND", "Board not found");
        }

        if (task_board->user_id_ != user_id) {
            spdlog::error("Task edit rejected: board with id={} belongs to another user",
                          existing_task->board_id_);
            return server::utils::build_error_response(req, http::status::forbidden,
                                                       "RESOURCE_NOT_OWNED",
                                                       "Resource belongs to another user");
        }

        std::string title = existing_task->title_;
        if (body.contains("title")) {
            title = body["title"].get<std::string>();
            if (title.empty() || title.size() > 100) {
                return server::utils::build_error_response(
                    req, http::status::bad_request, "VALIDATION_ERROR", "Validation failed",
                    json{{"title", "Title must be between 1 and 100 characters"}});
            }
        }

        std::string description = existing_task->description_;
        if (body.contains("description")) {
            description = body["description"].get<std::string>();
            if (description.size() > 1000) {
                return server::utils::build_error_response(
                    req, http::status::bad_request, "VALIDATION_ERROR", "Validation failed",
                    json{{"description", "Description cannot exceed 1000 characters"}});
            }
        }

        int status_id = existing_task->status_id_;
        if (body.contains("status_id")) {
            status_id = body["status_id"].get<int>();
            if (status_id <= 0) {
                return server::utils::build_error_response(
                    req, http::status::bad_request, "VALIDATION_ERROR", "Validation failed",
                    json{{"status_id", "Field must be a positive integer"}});
            }

            StatusRepository status_repository(pool);
            const std::optional<Status> new_status = status_repository.find_by_id(status_id);
            if (!new_status.has_value()) {
                return server::utils::build_error_response(req, http::status::not_found,
                                                           "STATUS_NOT_FOUND", "Status not found");
            }

            if (new_status->board_id_ != existing_task->board_id_) {
                return server::utils::build_error_response(
                    req, http::status::bad_request, "VALIDATION_ERROR", "Validation failed",
                    json{{"status_id", "status_id must reference a status from this board"}});
            }
        }

        std::string priority_color = existing_task->priority_color_;
        if (body.contains("priority_color")) {
            priority_color = body["priority_color"].get<std::string>();
            if (priority_color.empty() || priority_color.size() > 50) {
                return server::utils::build_error_response(
                    req, http::status::bad_request, "VALIDATION_ERROR", "Validation failed",
                    json{{"priority_color", "Priority color must be between 1 and 50 characters"}});
            }
        }

        bool is_completed = existing_task->is_completed_;
        if (body.contains("is_completed")) {
            is_completed = body["is_completed"].get<bool>();
        }

        std::optional<std::time_t> deadline = existing_task->deadline_;
        if (body.contains("deadline")) {
            if (body["deadline"].is_null()) {
                deadline = std::nullopt;
            } else if (body["deadline"].is_string()) {
                deadline = parse_iso8601_utc(body["deadline"].get<std::string>());
            } else {
                throw std::invalid_argument("type:deadline");
            }
        }

        const Task task_to_save(existing_task->id_, existing_task->board_id_, title, description,
                                deadline, status_id, priority_color, is_completed,
                                existing_task->created_at_, existing_task->updated_at_);

        const Task updated_task = task_repository.save(task_to_save);

        spdlog::info("Task with id={} successfully edited", task_id);
        return server::utils::build_json_response(req, http::status::ok,
                                                  json{{"data", model_to_json(updated_task)}});
    } catch (const std::invalid_argument& e) {
        const std::string message = e.what();

        if (message.rfind("missing:", 0) == 0) {
            spdlog::error("Task edit rejected: missing required fields");
            const std::string field = message.substr(8);
            return server::utils::build_error_response(req, http::status::bad_request,
                                                       "MISSING_FIELD", "Missing required fields",
                                                       json{{field, "Missing required field"}});
        }

        if (message.rfind("type:", 0) == 0) {
            spdlog::error("Task edit rejected: invalid field format");
            const std::string field = message.substr(5);
            return server::utils::build_error_response(
                req, http::status::bad_request, "INVALID_FORMAT", "Invalid field format",
                json{{field, "Invalid " + field + " format"}});
        }

        if (message.rfind("value:", 0) == 0) {
            spdlog::error("Task edit rejected: invalid field value");
            const std::string field = message.substr(6);
            return server::utils::build_error_response(
                req, http::status::bad_request, "VALIDATION_ERROR", "Validation failed",
                json{{field, "Field must be a positive integer"}});
        }

        return server::utils::build_error_response(req, http::status::bad_request,
                                                   "VALIDATION_ERROR", "Validation failed");
    } catch (const std::runtime_error& e) {
        spdlog::error("Task edit failed with database error: {}", e.what());
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "DATABASE_ERROR", "Database error");
    } catch (const std::exception& e) {
        spdlog::error("Task edit failed with unexpected error: {}", e.what());
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "INTERNAL_ERROR", "Internal server error");
    }
}

} // namespace task::v1
