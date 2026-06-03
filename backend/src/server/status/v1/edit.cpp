#include "edit.hpp"

#include "../../../../repositories/board_repository.hpp"
#include "../../../../repositories/status_repository.hpp"
#include "../../utils/response_utils.hpp"

#include <nlohmann/json.hpp>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>

using json = nlohmann::json;

namespace status::v1 {
namespace {

const size_t MAX_NAME_SIZE = 50;

json missing_fields(const json& body) {
    json missing = json::array();

    if (!body.contains("status_id")) {
        missing.push_back("status_id");
    }
    if (!body.contains("name")) {
        missing.push_back("name");
    }
    if (!body.contains("position")) {
        missing.push_back("position");
    }

    return missing;
}

int positive_field(const json& body, const std::string& key) {
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

int negative_field(const json& body, const std::string& key) {
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

std::string string_field(const json& body, const std::string& key) {
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
} // namespace

auto handleEdit(const http::request<http::string_body>& req, ConnectionPool& pool,
                int user_id) -> http::response<http::string_body> {
    spdlog::info("Status edit request received");
    if (req.method() != http::verb::patch) {
        spdlog::error("Status edit rejected: method not allowed");
        return server::utils::build_error_response(req, http::status::method_not_allowed,
                                                   "DUPLICATE_RESOURCE", "Method not allowed");
    }

    json body;
    try {
        body = json::parse(req.body());
    } catch (const json::exception&) {
        spdlog::error("Status edit rejected: invalid JSON format");
        return server::utils::build_error_response(req, http::status::bad_request, "INVALID_FORMAT",
                                                   "Invalid JSON format");
    }

    if (!body.is_object()) {
        spdlog::error("Status edit rejected: invalid JSON format");
        return server::utils::build_error_response(req, http::status::bad_request, "INVALID_FORMAT",
                                                   "Invalid JSON format");
    }

    const json missing = missing_fields(body);
    if (!missing.empty()) {
        spdlog::error("Status edit rejected: missing required fields");
        return server::utils::build_error_response(req, http::status::bad_request, "MISSING_FIELD",
                                                   "Missing required fields",
                                                   json{{"missing_fields", missing}});
    }

    try {
        const int status_id = positive_field(body, "status_id");
        const std::string name = string_field(body, "name");
        const int position = negative_field(body, "position");

        if (name.empty() || name.size() > MAX_NAME_SIZE) {
            spdlog::error("Status edit rejected: invalid name format");
            return server::utils::build_error_response(
                req, http::status::bad_request, "VALIDATION_ERROR", "Validation failed",
                json{{"name", "Name length must be between 1 and 50 symbols"}});
        }

        StatusRepository status_repository(pool);
        const std::optional<Status> old_status = status_repository.find_by_id(status_id);
        if (!old_status.has_value()) {
            spdlog::error("Status edit rejected: status with id={} not found", status_id);
            return server::utils::build_error_response(req, http::status::not_found,
                                                       "STATUS_NOT_FOUND", "Status not found");
        }

        BoardRepository board_repository(pool);
        const std::optional<Board> board = board_repository.find_by_id(old_status->board_id_);
        if (!board.has_value()) {
            spdlog::error("Status edit rejected: board with id={} not found",
                          old_status->board_id_);
            return server::utils::build_error_response(req, http::status::not_found,
                                                       "STATUS_NOT_FOUND", "Status not found");
        }

        if (board->user_id_ != user_id) {
            spdlog::error("Status edit rejected: board with id={} belongs to another user",
                          old_status->board_id_);
            return server::utils::build_error_response(req, http::status::forbidden,
                                                       "RESOURCE_NOT_OWNED",
                                                       "Resource belongs to another user");
        }

        const std::optional<Status> existing_status =
            status_repository.find_by_board_and_name(old_status->board_id_, name);

        if (existing_status.has_value() && existing_status->id_ != status_id) {
            spdlog::error("Status edit rejected: status with this name already exists");
            return server::utils::build_error_response(
                req, static_cast<http::status>(405), "DUPLICATE_RESOURCE",
                "Status with this name already exists", json{{"name", "already exists"}});
        }

        const Status status_to_save(status_id, old_status->board_id_, name, position);
        const Status updated_status = status_repository.save(status_to_save);

        spdlog::info("Status with id={} successfully edited", status_id);
        return server::utils::build_json_response(req, http::status::ok,
                                                  json{{"data", model_to_json(updated_status)}});
    } catch (const std::invalid_argument& e) {
        const std::string message = e.what();

        if (message.rfind("missing:", 0) == 0) {
            spdlog::error("Status edit rejected: missing required fields");
            const std::string field = message.substr(8);
            return server::utils::build_error_response(
                req, http::status::bad_request, "MISSING_FIELD", "Missing required fields",
                json{{"missing_fields", json::array({field})}});
        }

        if (message.rfind("type:", 0) == 0) {
            spdlog::error("Status edit rejected: invalid field format");
            const std::string field = message.substr(5);
            return server::utils::build_error_response(
                req, http::status::bad_request, "INVALID_FORMAT", "Invalid field format",
                json{{field, "Invalid " + field + " format"}});
        }

        if (message.rfind("value:", 0) == 0) {
            spdlog::error("Status edit rejected: invalid field value");
            const std::string field = message.substr(6);
            const std::string detail = field == "position"
                                           ? "Position must be greater than or equal to 0"
                                           : "Status id must be positive";
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
            spdlog::error("Status edit rejected: status with this name already exists");
            return server::utils::build_error_response(
                req, static_cast<http::status>(405), "DUPLICATE_RESOURCE",
                "Status with this name already exists", json{{"name", "already exists"}});
        }

        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "DATABASE_ERROR", "Database error");
    } catch (const std::runtime_error& e) {
        spdlog::error("Status edit failed with database error: {}", e.what());
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "DATABASE_ERROR", e.what());
    } catch (const std::exception& e) {
        spdlog::error("Status edit failed with unexpected error: {}", e.what());
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "INTERNAL_ERROR", "Internal server error");
    }
}

} // namespace status::v1
