#include "create.hpp"

#include "../../../../repositories/board_repository.hpp"
#include "../../../../repositories/status_repository.hpp"
#include "../../utils/response_utils.hpp"

#include <nlohmann/json.hpp>
#include <optional>
#include <pqxx/pqxx>
#include <string>

using json = nlohmann::json;

namespace status::v1 {

namespace {

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

int require_non_negative_int_field(const json& body, const std::string& key) {
    try {
        const int value = body.at(key).get<int>();
        if (value < 0) {
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

json model_to_json(const Status& status) {
    return json{{"id", status.id_},
                {"board_id", status.board_id_},
                {"name", status.name_},
                {"position", status.position_}};
}

json collect_missing_fields(const json& body) {
    json missing = json::array();

    if (!body.contains("board_id")) {
        missing.push_back("board_id");
    }
    if (!body.contains("name")) {
        missing.push_back("name");
    }
    if (!body.contains("position")) {
        missing.push_back("position");
    }

    return missing;
}

} // namespace

auto handleCreate(const http::request<http::string_body>& req, ConnectionPool& pool,
                  int user_id) -> http::response<http::string_body> {

    if (req.method() != http::verb::post) {
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
        const int board_id = require_positive_int_field(body, "board_id");
        const std::string name = require_string_field(body, "name");
        const int position = require_non_negative_int_field(body, "position");

        if (name.empty() || name.size() > 50) {
            return server::utils::build_error_response(
                req, http::status::bad_request, "VALIDATION_ERROR", "Validation failed",
                json{{"name", "Name length must be between 1 and 50 symbols"}});
        }

        BoardRepository board_repository(pool);
        const std::optional<Board> board = board_repository.find_by_id(board_id);

        if (!board.has_value()) {
            return server::utils::build_error_response(req, http::status::not_found,
                                                       "BOARD_NOT_FOUND", "Board not found");
        }

        if (board->user_id_ != user_id) {
            return server::utils::build_error_response(req, http::status::forbidden,
                                                       "RESOURCE_NOT_OWNED",
                                                       "Resource belongs to another user");
        }

        StatusRepository status_repository(pool);
        const std::optional<Status> existing_status =
            status_repository.find_by_board_and_name(board_id, name);

        if (existing_status.has_value()) {
            return server::utils::build_error_response(
                req, static_cast<http::status>(405), "DUPLICATE_RESOURCE",
                "Status with this name already exists", json{{"name", "already exists"}});
        }

        const Status created_status = status_repository.create(board_id, name, position);

        return server::utils::build_json_response(req, http::status::ok,
                                                  json{{"data", model_to_json(created_status)}});

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
            const std::string detail = field == "position"
                                           ? "Position must be greater than or equal to 0"
                                           : "Board id must be positive";
            return server::utils::build_error_response(req, http::status::bad_request,
                                                       "VALIDATION_ERROR", "Validation failed",
                                                       json{{field, detail}});
        }

        return server::utils::build_error_response(req, http::status::bad_request,
                                                   "VALIDATION_ERROR", "Validation failed");
    } catch (const pqxx::sql_error& e) {
        const std::string msg = e.what();
        if (msg.find("statuses_board_id_name_key") != std::string::npos ||
            msg.find("duplicate key") != std::string::npos) {
            return server::utils::build_error_response(
                req, static_cast<http::status>(405), "DUPLICATE_RESOURCE",
                "Status with this name already exists", json{{"name", "already exists"}});
        }

        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "DATABASE_ERROR", "Database error");
    } catch (const std::exception&) {
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "DATABASE_ERROR", "Database error");
    }
}

} // namespace status::v1
