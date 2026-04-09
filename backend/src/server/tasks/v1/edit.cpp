#include "edit.hpp"

#include "../../../../repositories/status_repository.hpp"
#include "../../../../repositories/task_repository.hpp"

#include <array>
#include <ctime>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>

using json = nlohmann::json;

namespace tasks::v1 {

namespace {

json parse_body(const http::request<http::string_body>& req) {
    json body = json::parse(req.body());
    if (!body.is_object()) {
        throw std::invalid_argument("Request body must be a JSON object");
    }
    return body;
}

bool has_editable_fields(const json& body) {
    return body.contains("title") || body.contains("description") || body.contains("status") ||
           body.contains("priority_color") || body.contains("deadline");
}

std::string time_to_string_iso8601(std::time_t t) {
    std::array<char, 25> buffer{};
    if (std::strftime(buffer.data(), buffer.size(), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t))) {
        return {buffer.data()};
    }
    return "";
}

std::optional<std::time_t> iso_8601_to_time(const std::string& str) {
    std::tm tm{};
    std::istringstream input(str);
    input >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    if (input.fail()) {
        throw std::invalid_argument("Invalid deadline format");
    }
    return timegm(&tm);
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

json model_to_json(const Task& task, const std::string& status_name) {
    json data = {
        {"id", task.id_},
        {"board_id", task.board_id_},
        {"title", task.title_},
        {"description", task.description_},
        {"status", status_name},
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

auto handleEdit(const http::request<http::string_body>& req, ConnectionPool& pool)
    -> http::response<http::string_body> {
    try {
        TaskRepository task_repository(pool);
        StatusRepository status_repository(pool);
        json body = parse_body(req);

        if (!body.contains("task_id")) {
            return build_error_response(req, http::status::bad_request, "MISSING_FIELD",
                                        "task_id is required");
        }
        if (!body["task_id"].is_number_integer()) {
            return build_error_response(req, http::status::bad_request, "INVALID_FORMAT",
                                        "task_id must be an integer");
        }
        if (!has_editable_fields(body)) {
            return build_error_response(req, http::status::bad_request, "MISSING_FIELD",
                                        "At least one editable field is required");
        }

        int task_id = body["task_id"].get<int>();

        auto task_opt = task_repository.find_by_id(task_id);
        if (!task_opt) {
            return build_error_response(req, http::status::not_found, "TASK_NOT_FOUND",
                                        "Task not found");
        }

        Task task = task_opt.value();

        if (body.contains("title")) {
            task.title_ = body["title"].get<std::string>();
        }
        if (body.contains("description")) {
            task.description_ = body["description"].get<std::string>();
        }
        if (body.contains("status")) {
            auto status_opt = status_repository.find_by_board_and_name(
                task.board_id_, body["status"].get<std::string>());
            if (!status_opt) {
                return build_error_response(req, http::status::bad_request, "VALIDATION_ERROR",
                                            "Unknown status for board",
                                            json{{"status", body["status"]}});
            }
            task.status_id_ = status_opt->id_;
        }
        if (body.contains("priority_color")) {
            task.priority_color_ = body["priority_color"].get<std::string>();
        }
        if (body.contains("deadline")) {
            if (body["deadline"].is_null()) {
                task.deadline_ = std::nullopt;
            } else if (body["deadline"].is_string()) {
                task.deadline_ = iso_8601_to_time(body["deadline"].get<std::string>());
            } else {
                return build_error_response(req, http::status::bad_request, "INVALID_FORMAT",
                                            "deadline must be a string or null");
            }
        }

        Task updated_task = task_repository.save(task);
        auto status_opt = status_repository.find_by_id(updated_task.status_id_);
        if (!status_opt) {
            return build_error_response(req, http::status::internal_server_error, "DATABASE_ERROR",
                                        "Status not found after task update");
        }

        return build_json_response(req, http::status::ok,
                                   json{{"data", model_to_json(updated_task, status_opt->name_)}});
    } catch (const std::invalid_argument& e) {
        return build_error_response(req, http::status::bad_request, "VALIDATION_ERROR", e.what());
    } catch (const nlohmann::json::exception&) {
        return build_error_response(req, http::status::bad_request, "INVALID_FORMAT",
                                    "Invalid JSON body");
    } catch (const std::exception& e) {
        return build_error_response(req, http::status::internal_server_error, "INTERNAL_ERROR",
                                    e.what());
    }
}

} // namespace tasks::v1
