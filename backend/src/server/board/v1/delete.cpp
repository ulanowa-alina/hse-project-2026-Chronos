#include "delete.hpp"

#include "../../../../repositories/board_repository.hpp"
#include "../../utils/response_utils.hpp"

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

namespace board::v1 {

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

} // namespace

auto handleDelete(const http::request<http::string_body>& req, ConnectionPool& pool,
                  int user_id) -> http::response<http::string_body> {
    spdlog::info("Board delete request received");
    if (req.method() != http::verb::delete_) {
        spdlog::warn("Board delete rejected: method not allowed");
        return server::utils::build_error_response(req, http::status::method_not_allowed,
                                                   "DUPLICATE_RESOURCE", "Method not allowed");
    }

    json body;
    try {
        body = json::parse(req.body());
    } catch (const json::exception&) {
        spdlog::warn("Board delete rejected: invalid JSON format");
        return server::utils::build_error_response(req, http::status::bad_request, "INVALID_FORMAT",
                                                   "Request body contains invalid JSON");
    }

    if (!body.is_object()) {
        spdlog::warn("Board delete rejected: invalid JSON format");
        return server::utils::build_error_response(req, http::status::bad_request, "INVALID_FORMAT",
                                                   "Request body must be a JSON object");
    }

    try {
        const int board_id = require_int_field(body, "board_id");

        BoardRepository board_repository(pool);
        const std::optional<Board> board = board_repository.find_by_id(board_id);
        if (!board.has_value()) {
            spdlog::warn("Board delete rejected: board with id = {} not found", board_id);
            return server::utils::build_error_response(req, http::status::not_found,
                                                       "BOARD_NOT_FOUND", "Board not found");
        }

        if (board->user_id_ != user_id) {
            spdlog::warn("Board delete rejected: board with id = {} belongs to another user", board_id);
            return server::utils::build_error_response(req, http::status::forbidden,
                                                       "RESOURCE_NOT_OWNED",
                                                       "Resource belongs to another user");
        }

        if (!board_repository.delete_by_id(board_id)) {
            spdlog::warn("Board delete rejected: board with id = {} not found", board_id);
            return server::utils::build_error_response(req, http::status::not_found,
                                                       "BOARD_NOT_FOUND", "Board not found");
        }

        http::response<http::string_body> res{http::status::no_content, req.version()};
        res.set(http::field::access_control_allow_origin, "*");
        res.keep_alive(req.keep_alive());
        spdlog::info("Board with id={} successfully deleted", board_id);
        return res;
    } catch (const std::invalid_argument& e) {
        const std::string message = e.what();
        if (message.rfind("missing:", 0) == 0) {
            spdlog::warn("Board delete rejected: missing required fields");

            const std::string field = message.substr(8);
            return server::utils::build_error_response(
                req, http::status::bad_request, "MISSING_FIELD", "Missing required field",
                json{{field, "Field " + field + " is required"}});
        }
        if (message.rfind("type:", 0) == 0) {
            spdlog::warn("Board delete rejected: invalid filed format");

            const std::string field = message.substr(5);
            return server::utils::build_error_response(
                req, http::status::bad_request, "INVALID_FORMAT", "Invalid field format",
                json{{field, "Field " + field + " has invalid type"}});
        }
        if (message.rfind("value:", 0) == 0) {
            spdlog::warn("Board delete rejected: invalid field value");
            const std::string field = message.substr(6);
            return server::utils::build_error_response(
                req, http::status::bad_request, "VALIDATION_ERROR", "Invalid field value",
                json{{field, "Field " + field + " must be a positive integer"}});
        }
        return server::utils::build_error_response(req, http::status::bad_request,
                                                   "VALIDATION_ERROR", e.what());
    } catch (const std::runtime_error& e) {
        spdlog::error("Board delete failed with database error: {}", e.what());
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "DATABASE_ERROR", e.what());
    } catch (const std::exception& e) {
        spdlog::error("Board delete failed with unexpected error: {}", e.what());
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "INTERNAL_ERROR", "Internal server error");
    }
}

} // namespace board::v1
