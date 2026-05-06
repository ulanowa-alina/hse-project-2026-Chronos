#include "create.hpp"

#include "../../../../repositories/board_repository.hpp"
#include "../../utils/response_utils.hpp"

#include <array>
#include <ctime>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

using json = nlohmann::json;

namespace board::v1 {

namespace {

std::string time_to_string_iso8601(std::time_t t) {
    std::array<char, 25> buffer{};
    if (std::strftime(buffer.data(), buffer.size(), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t))) {
        return {buffer.data()};
    }
    throw std::runtime_error("Failed to format timestamp");
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

bool require_bool_field(const json& body, const std::string& key) {
    try {
        return body.at(key).get<bool>();
    } catch (const json::out_of_range&) {
        throw std::invalid_argument("missing:" + key);
    } catch (const json::type_error&) {
        throw std::invalid_argument("type:" + key);
    }
}

json collect_missing_fields(const json& body) {
    json missing = json::array();

    if (!body.contains("title")) {
        missing.push_back("title");
    }
    if (!body.contains("is_private")) {
        missing.push_back("is_private");
    }

    return missing;
}

json model_to_json(const Board& board) {
    return json{{"id", board.id_},
                {"user_id", board.user_id_},
                {"title", board.title_},
                {"description", board.description_},
                {"is_private", board.is_private_},
                {"created_at", time_to_string_iso8601(board.created_at_)},
                {"updated_at", time_to_string_iso8601(board.updated_at_)}};
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
        const std::string title = require_string_field(body, "title");
        const std::string description = optional_string_field(body, "description");
        const bool is_private = require_bool_field(body, "is_private");

        if (title.empty() || title.size() > 100) {
            return server::utils::build_error_response(
                req, http::status::bad_request, "VALIDATION_ERROR", "Validation failed",
                json{{"title", "Title length must be between 1 and 100 symbols"}});
        }

        if (description.size() > 1000) {
            return server::utils::build_error_response(
                req, http::status::bad_request, "VALIDATION_ERROR", "Validation failed",
                json{{"description", "Description cannot exceed 1000 symbols"}});
        }

        BoardRepository board_repository(pool);
        const Board new_board(0, user_id, title, description, is_private, 0, 0);
        const Board created_board = board_repository.save(new_board);

        return server::utils::build_json_response(req, http::status::ok,
                                                  json{{"data", model_to_json(created_board)}});
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

} // namespace board::v1
