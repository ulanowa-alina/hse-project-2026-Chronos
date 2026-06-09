#include "get_user_sessions.hpp"

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

auto handleGetUserSessions(const http::request<http::string_body>& req, ConnectionPool& pool,
                           int user_id) -> http::response<http::string_body> {
    if (req.method() != http::verb::get) {
        return server::utils::build_error_response(req, http::status::method_not_allowed,
                                                   "METHOD_NOT_ALLOWED",
                                                   "Only GET is supported for this endpoint");
    }

    try {
        PomodoroSessionRepository repository(pool);
        std::vector<PomodoroSession> sessions = repository.find_by_user_id(user_id);

        json sessions_array = json::array();
        for (const auto& session : sessions) {
            sessions_array.push_back(model_to_json(session));
        }

        return server::utils::build_json_response(req, http::status::ok,
                                                  json{{"data", sessions_array}});
    } catch (const std::runtime_error& e) {
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "DATABASE_ERROR", e.what());
    } catch (const std::exception& e) {
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "INTERNAL_ERROR", e.what());
    }
}

} // namespace pomodoro::v1
