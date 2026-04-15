#include "edit.hpp"

#include "../../../../repositories/board_repository.hpp"
#include "../../../../repositories/status_repository.hpp"
#include "../../../../repositories/task_repository.hpp"
#include "../../utils/response_utils.hpp"

#include <array>
#include <ctime>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>

using json = nlohmann::json;

namespace task::v1 {

namespace {

json collect_missing_fields(const json& body) {
    json missing = json::array();

    if (!body.contains("task_id")) {
        missing.push_back("task_id");
    }
    if (!body.contains("title")) {
        missing.push_back("title");
    }
    if (!body.contains("description")) {
        missing.push_back("description");
    }
    if (!body.contains("status_id")) {
        missing.push_back("status_id");
    }
    if (!body.contains("priority_color")) {
        missing.push_back("priority_color");
    }

    return missing;
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

json model_to_json(const Task& task) {
    json data = {{"id", task.id_},
                 {"board_id", task.board_id_},
                 {"title", task.title_},
                 {"description", task.description_},
                 {"status_id", task.status_id_},
                 {"priority_color", task.priority_color_},
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

    if (req.method() != http::verb::patch) {
        return server::utils::build_error_response(req, http::status::method_not_allowed,
                                                   "DUPLICATE_RESOURCE", "Method not allowed");
    }

    json body;
    try {
        body = json::parse(req.body());
    } catch (const json::exception&) {
        return server::utils::build_error_response(req, http::status::bad_request, "INVALID_FORMAT",
                                                   "Invalid JSON format");
    }

    if (!body.is_object()) {
        return server::utils::build_error_response(req, http::status::bad_request, "INVALID_FORMAT",
                                                   "Invalid JSON format");
    }

    const json missing_fields = collect_missing_fields(body);
    if (!missing_fields.empty()) {
        return server::utils::build_error_response(req, http::status::bad_request, "MISSING_FIELD",
                                                   "Missing required fields",
                                                   json{{"missing_fields", missing_fields}});
    }

    try {
        const int task_id = require_positive_int_field(body, "task_id");
        const std::string title = require_string_field(body, "title");
        const std::string description = require_string_field(body, "description");
        const int status_id = require_positive_int_field(body, "status_id");
        const std::string priority_color = require_string_field(body, "priority_color");

        if (title.empty() || title.size() > 100) {
            return server::utils::build_error_response(
                req, http::status::bad_request, "VALIDATION_ERROR", "Validation failed",
                json{{"title", "Title must be between 1 and 100 characters"}});
        }

        if (description.size() > 1000) {
            return server::utils::build_error_response(
                req, http::status::bad_request, "VALIDATION_ERROR", "Validation failed",
                json{{"description", "Description cannot exceed 1000 characters"}});
        }

        if (priority_color.empty() || priority_color.size() > 50) {
            return server::utils::build_error_response(
                req, http::status::bad_request, "VALIDATION_ERROR", "Validation failed",
                json{{"priority_color", "Priority color must be between 1 and 50 characters"}});
        }

        TaskRepository task_repository(pool);
        const std::optional<Task> existing_task = task_repository.find_by_id(task_id);
        if (!existing_task.has_value()) {
            return server::utils::build_error_response(req, http::status::not_found,
                                                       "TASK_NOT_FOUND", "Task not found");
        }

        BoardRepository board_repository(pool);
        const std::optional<Board> task_board =
            board_repository.find_by_id(existing_task->board_id_);
        if (!task_board.has_value()) {
            return server::utils::build_error_response(req, http::status::not_found,
                                                       "BOARD_NOT_FOUND", "Board not found");
        }

        if (task_board->user_id_ != user_id) {
            return server::utils::build_error_response(req, http::status::forbidden,
                                                       "RESOURCE_NOT_OWNED",
                                                       "Resource belongs to another user");
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

        const Task task_to_save(existing_task->id_, existing_task->board_id_, title, description,
                                existing_task->deadline_, status_id, priority_color,
                                existing_task->created_at_, existing_task->updated_at_);

        const Task updated_task = task_repository.save(task_to_save);

        return server::utils::build_json_response(req, http::status::ok,
                                                  json{{"data", model_to_json(updated_task)}});
    } catch (const std::invalid_argument& e) {
        const std::string message = e.what();

        if (message.rfind("missing:", 0) == 0) {
            const std::string field = message.substr(8);
            return server::utils::build_error_response(
                req, http::status::bad_request, "MISSING_FIELD", "Missing required fields",
                json{{"missing_fields", json::array({field})}});
        }

        if (message.rfind("type:", 0) == 0) {
            const std::string field = message.substr(5);
            return server::utils::build_error_response(
                req, http::status::bad_request, "INVALID_FORMAT", "Invalid field format",
                json{{field, "Invalid " + field + " format"}});
        }

        if (message.rfind("value:", 0) == 0) {
            const std::string field = message.substr(6);
            return server::utils::build_error_response(
                req, http::status::bad_request, "VALIDATION_ERROR", "Validation failed",
                json{{field, "Field must be a positive integer"}});
        }

        return server::utils::build_error_response(req, http::status::bad_request,
                                                   "VALIDATION_ERROR", "Validation failed");
    } catch (const std::runtime_error&) {
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "DATABASE_ERROR", "Database error");
    } catch (const std::exception&) {
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "INTERNAL_ERROR", "Internal server error");
    }
}

} // namespace task::v1
