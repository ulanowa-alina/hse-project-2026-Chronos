#include "get_all.hpp"

#include "../../../../repositories/board_repository.hpp"
#include "../../../../repositories/status_repository.hpp"
#include "../../utils/response_utils.hpp"

#include <boost/url.hpp>
#include <limits>
#include <nlohmann/json.hpp>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>

using json = nlohmann::json;

namespace status::v1 {

namespace {

json model_to_json(const Status& status) {
    return json{{"id", status.id_},
                {"board_id", status.board_id_},
                {"name", status.name_},
                {"position", status.position_}};
}

std::optional<int> parse_int_param(const boost::urls::params_view& params, const std::string& key,
                                   bool& present) {
    const auto param = params.find(key);
    if (param == params.end()) {
        present = false;
        return std::nullopt;
    }

    present = true;

    try {
        std::size_t parsed_chars = 0;
        const std::string value((*param).value);
        const auto parsed = std::stoll(value, &parsed_chars);

        if (parsed_chars != value.size()) {
            throw std::invalid_argument("type:" + key);
        }
        if (parsed <= 0 || parsed > std::numeric_limits<int>::max()) {
            throw std::invalid_argument("value:" + key);
        }

        return static_cast<int>(parsed);
    } catch (const std::invalid_argument& e) {
        const std::string msg = e.what();
        if (msg.rfind("type:", 0) == 0 || msg.rfind("value:", 0) == 0) {
            throw;
        }
        throw std::invalid_argument("type:" + key);
    } catch (const std::out_of_range&) {
        throw std::invalid_argument("value:" + key);
    } catch (const std::exception&) {
        throw std::invalid_argument("type:" + key);
    }
}

} // namespace

auto handleGetAll(const http::request<http::string_body>& req, ConnectionPool& pool,
                  int user_id) -> http::response<http::string_body> {
    spdlog::info("Status get all request received");
    if (req.method() != http::verb::get) {
        spdlog::error("Status get all rejected: method not allowed");
        return server::utils::build_error_response(req, http::status::method_not_allowed,
                                                   "METHOD_NOT_ALLOWED",
                                                   "Only GET is supported for this endpoint");
    }

    try {
        const auto url_view_result = boost::urls::parse_origin_form(req.target());
        if (!url_view_result) {
            spdlog::error("Status get all rejected: Invalid field format");
            return server::utils::build_error_response(req, http::status::bad_request,
                                                       "INVALID_FORMAT", "Invalid field format",
                                                       json{{"query", "Invalid query format"}});
        }

        const auto params = url_view_result->params();
        bool has_board_id = false;
        const std::optional<int> board_id = parse_int_param(params, "board_id", has_board_id);

        StatusRepository status_repository(pool);
        std::vector<Status> statuses;

        if (board_id.has_value()) {
            spdlog::info("Status get all for board request received");
            BoardRepository board_repository(pool);
            const std::optional<Board> board = board_repository.find_by_id(*board_id);
            if (!board.has_value()) {
                spdlog::error("Status get all rejected: board with id={} not found",
                              board_id.value());
                return server::utils::build_error_response(req, http::status::not_found,
                                                           "BOARD_NOT_FOUND", "Board not found");
            }

            if (board->user_id_ != user_id) {
                spdlog::error("Status get all rejected: board with id={} belongs to another user",
                              board->id_);
                return server::utils::build_error_response(req, http::status::forbidden,
                                                           "RESOURCE_NOT_OWNED",
                                                           "Resource belongs to another user");
            }

            statuses = status_repository.find_by_board_id(*board_id);
        } else {
            spdlog::info("Status get all for user request received");
            statuses = status_repository.find_by_user_id(user_id);
        }

        json data = json::array();
        for (const auto& status : statuses) {
            data.push_back(model_to_json(status));
        }
        spdlog::info("Status get all succeeded with status_count={}", statuses.size());
        return server::utils::build_json_response(req, http::status::ok, json{{"data", data}});
    } catch (const std::invalid_argument& e) {
        const std::string message = e.what();

        if (message.rfind("type:", 0) == 0) {
            spdlog::error("Status get all rejected: Invalid field format");
            const std::string field = message.substr(5);
            return server::utils::build_error_response(
                req, http::status::bad_request, "INVALID_FORMAT", "Invalid field format",
                json{{field, "Field " + field + " has invalid type"}});
        }

        if (message.rfind("value:", 0) == 0) {
            spdlog::error("Status get all rejected: Invalid field value");
            const std::string field = message.substr(6);
            return server::utils::build_error_response(
                req, http::status::bad_request, "VALIDATION_ERROR", "Invalid field value",
                json{{field, "Field " + field + " must be a positive integer"}});
        }

        spdlog::error("Status get all rejected: validation error");
        return server::utils::build_error_response(req, http::status::bad_request,
                                                   "VALIDATION_ERROR", message);
    } catch (const std::runtime_error& e) {
        spdlog::error("Status get all failed with database error: {}", e.what());
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "DATABASE_ERROR", "Database error");
    } catch (const std::exception& e) {
        spdlog::error("Status get all failed with unexpected error: {}", e.what());
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "INTERNAL_ERROR", "Internal server error");
    }
}

} // namespace status::v1
