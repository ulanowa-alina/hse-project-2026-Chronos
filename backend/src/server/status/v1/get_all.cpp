#include "get_all.hpp"

#include "../../../../repositories/board_repository.hpp"
#include "../../../../repositories/status_repository.hpp"
#include "../../utils/response_utils.hpp"

#include <boost/url.hpp>
#include <limits>
#include <nlohmann/json.hpp>
#include <optional>
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
    if (req.method() != http::verb::get) {
        return server::utils::build_error_response(req, http::status::method_not_allowed,
                                                   "METHOD_NOT_ALLOWED",
                                                   "Only GET is supported for this endpoint");
    }

    try {
        const auto url_view_result = boost::urls::parse_origin_form(req.target());
        if (!url_view_result) {
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
            BoardRepository board_repository(pool);
            const std::optional<Board> board = board_repository.find_by_id(*board_id);
            if (!board.has_value()) {
                return server::utils::build_error_response(req, http::status::not_found,
                                                           "BOARD_NOT_FOUND", "Board not found");
            }

            if (board->user_id_ != user_id) {
                return server::utils::build_error_response(req, http::status::forbidden,
                                                           "RESOURCE_NOT_OWNED",
                                                           "Resource belongs to another user");
            }

            statuses = status_repository.find_by_board_id(*board_id);
        } else {
            statuses = status_repository.find_by_user_id(user_id);
        }

        json data = json::array();
        for (const auto& status : statuses) {
            data.push_back(model_to_json(status));
        }

        return server::utils::build_json_response(req, http::status::ok, json{{"data", data}});
    } catch (const std::invalid_argument& e) {
        const std::string message = e.what();

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
                                                   "VALIDATION_ERROR", message);
    } catch (const std::runtime_error& e) {
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "DATABASE_ERROR", e.what());
    } catch (const std::exception& e) {
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "INTERNAL_ERROR", e.what());
    }
}

} // namespace status::v1
