#include "delete.hpp"

#include "../../../../repositories/board_repository.hpp"
#include "../../../../repositories/task_repository.hpp"
#include "../../auth/jwt.hpp"
#include "../../utils/response_utils.hpp"

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

using json = nlohmann::json;

namespace task::v1 {

namespace {

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

std::string extract_bearer_token(const http::request<http::string_body>& req) {
    const http::request<http::string_body>::const_iterator auth =
        req.find(http::field::authorization);
    if (auth == req.end()) {
        throw std::runtime_error("unauthorized");
    }

    const std::string header(auth->value());
    const std::string prefix = "Bearer ";
    if (header.rfind(prefix, 0) != 0 || header.size() <= prefix.size()) {
        throw std::runtime_error("unauthorized");
    }

    return header.substr(prefix.size());
}

int require_authorized_user_id(const http::request<http::string_body>& req) {
    const std::string token = extract_bearer_token(req);

    auth::TokenPayload payload;
    auth::TokenError error = auth::TokenError::InvalidToken;
    if (!auth::parse_and_validate_token(token, payload, error) || payload.user_id <= 0) {
        throw std::runtime_error("unauthorized");
    }

    return payload.user_id;
}

} // namespace

auto handleDelete(const http::request<http::string_body>& req,
                  ConnectionPool& pool) -> http::response<http::string_body> {
    if (req.method() != http::verb::delete_) {
        return server::utils::build_error_response(req, http::status::method_not_allowed,
                                                   "METHOD_NOT_ALLOWED",
                                                   "Only DELETE is supported for this endpoint");
    }

    json body;
    try {
        body = json::parse(req.body());
    } catch (const json::exception&) {
        return server::utils::build_error_response(req, http::status::bad_request,
                                                   "INVALID_FORMAT",
                                                   "Request body contains invalid JSON");
    }

    if (!body.is_object()) {
        return server::utils::build_error_response(req, http::status::bad_request,
                                                   "INVALID_FORMAT",
                                                   "Request body must be a JSON object");
    }

    try {
        const int user_id = require_authorized_user_id(req);
        const int task_id = require_int_field(body, "task_id");

        TaskRepository task_repository(pool);
        const std::optional<Task> task = task_repository.find_by_id(task_id);
        if (!task.has_value()) {
            return server::utils::build_error_response(req, http::status::not_found,
                                                       "TASK_NOT_FOUND", "Task not found");
        }

        BoardRepository board_repository(pool);
        const std::optional<Board> board = board_repository.find_by_id(task->board_id_);
        if (!board.has_value()) {
            return server::utils::build_error_response(req, http::status::not_found,
                                                       "TASK_NOT_FOUND", "Task not found");
        }

        if (board->user_id_ != user_id) {
            return server::utils::build_error_response(req, http::status::forbidden,
                                                       "RESOURCE_NOT_OWNED",
                                                       "Resource belongs to another user");
        }

        if (!task_repository.delete_by_id(task_id)) {
            return server::utils::build_error_response(req, http::status::not_found,
                                                       "TASK_NOT_FOUND", "Task not found");
        }

        http::response<http::string_body> res{http::status::no_content, req.version()};
        res.set(http::field::access_control_allow_origin, "*");
        res.keep_alive(req.keep_alive());
        return res;
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
        if (std::string(e.what()) == "unauthorized") {
            return server::utils::build_error_response(req, http::status::unauthorized,
                                                       "UNAUTHORIZED",
                                                       "User is not authorized");
        }
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "DATABASE_ERROR", e.what());
    } catch (const std::exception& e) {
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "INTERNAL_ERROR", e.what());
    }
}

} // namespace task::v1
