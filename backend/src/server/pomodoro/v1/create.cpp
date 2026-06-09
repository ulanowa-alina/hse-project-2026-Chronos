#include "create.hpp"

#include "../../../../repositories/pomodoro_session_repository.hpp"
#include "../../utils/response_utils.hpp"

#include <array>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>
#include <string>

using json = nlohmann::json;

namespace pomodoro::v1 {

namespace {

std::string time_to_string_iso8601(std::time_t t) {
    std::array<char, 25> buffer{};
    if (std::strftime(buffer.data(), buffer.size(), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t))) {
        return {buffer.data()};
    }
    throw std::runtime_error("Failed to format timestamp");
}

std::optional<int> optional_int_field(const json& body, const std::string& key) {
    try {
        if (body.at(key).is_null()) {
            return std::nullopt;
        }
        const int value = body.at(key).get<int>();
        if (value <= 0) {
            throw std::invalid_argument("value:" + key);
        }
        return value;
    } catch (const json::out_of_range&) {
        return std::nullopt;
    } catch (const json::type_error&) {
        throw std::invalid_argument("type:" + key);
    }
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

json model_to_json(const PomodoroSession& session) {
    json data = {
        {"id", session.id_},
        {"user_id", session.user_id_},
        {"work_duration_seconds", session.work_duration_seconds_},
        {"break_duration_seconds", session.break_duration_seconds_},
        {"completed_cycles", session.completed_cycles_},
        {"started_at", time_to_string_iso8601(session.started_at_)},
    };

    if (session.goal_minutes_.has_value()) {
        data["goal_minutes"] = session.goal_minutes_.value();
    } else {
        data["goal_minutes"] = nullptr;
    }

    if (session.completed_at_.has_value()) {
        data["completed_at"] = time_to_string_iso8601(session.completed_at_.value());
    } else {
        data["completed_at"] = nullptr;
    }

    return data;
}

} // namespace

auto handleCreate(const http::request<http::string_body>& req, ConnectionPool& pool,
                  int user_id) -> http::response<http::string_body> {
    if (req.method() != http::verb::post) {
        return server::utils::build_error_response(req, http::status::method_not_allowed,
                                                   "METHOD_NOT_ALLOWED",
                                                   "Only POST is supported for this endpoint");
    }

    json body;
    try {
        body = json::parse(req.body());
    } catch (const json::exception&) {
        return server::utils::build_error_response(req, http::status::bad_request, "INVALID_FORMAT",
                                                   "Request body contains invalid JSON");
    }

    if (!body.is_object()) {
        return server::utils::build_error_response(req, http::status::bad_request, "INVALID_FORMAT",
                                                   "Request body must be a JSON object");
    }

    try {
        const std::optional<int> goal_minutes = optional_int_field(body, "goal_minutes");
        const int work_duration_seconds = require_int_field(body, "work_duration_seconds");
        const int break_duration_seconds = require_int_field(body, "break_duration_seconds");
        const int completed_cycles = require_int_field(body, "completed_cycles");

        PomodoroSessionRepository repository(pool);
        const std::time_t now =
            std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        PomodoroSession new_session(0, user_id, goal_minutes, work_duration_seconds,
                                    break_duration_seconds, completed_cycles, now, std::nullopt);
        const PomodoroSession created = repository.save(new_session);

        return server::utils::build_json_response(req, http::status::ok,
                                                  json{{"data", model_to_json(created)}});
    } catch (const std::invalid_argument& e) {
        const std::string message = e.what();

        if (message.rfind("missing:", 0) == 0) {
            const std::string field = message.substr(8);
            return server::utils::build_error_response(
                req, http::status::bad_request, "MISSING_FIELD", "Missing required field",
                json{{field, "Field " + field + " is required"}});
        }
        if (message.rfind("type:", 0) == 0) {
            const std::string field = message.substr(5);
            return server::utils::build_error_response(
                req, http::status::bad_request, "INVALID_FORMAT", "Invalid field format",
                json{{field, "Field " + field + " has invalid type"}});
        }
        if (message.rfind("value:", 0) == 0) {
            const std::string field = message.substr(6);
            return server::utils::build_error_response(
                req, http::status::bad_request, "VALIDATION_ERROR", "Invalid field value",
                json{{field, "Field " + field + " must be a positive integer"}});
        }
        return server::utils::build_error_response(req, http::status::bad_request,
                                                   "VALIDATION_ERROR", e.what());
    } catch (const std::runtime_error& e) {
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "DATABASE_ERROR", e.what());
    } catch (const std::exception& e) {
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "INTERNAL_ERROR", e.what());
    }
}

} // namespace pomodoro::v1
