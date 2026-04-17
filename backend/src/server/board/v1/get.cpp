#include "get.hpp"

#include "../../../../repositories/board_repository.hpp"
#include "../../utils/response_utils.hpp"

#include <array>
#include <boost/url.hpp>
#include <ctime>
#include <limits>
#include <nlohmann/json.hpp>
#include <optional>
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

std::optional<int> parse_board_id_query(const http::request<http::string_body>& req) {
    const auto url_view_result = boost::urls::parse_origin_form(req.target());
    if (!url_view_result) {
        throw std::invalid_argument("type:board_id");
    }

    const auto params = url_view_result->params();
    const auto it = params.find("board_id");
    if (it == params.end()) {
        return std::nullopt;
    }

    try {
        std::size_t parsed_chars = 0;
        const std::string value((*it).value);
        const auto parsed = std::stoll(value, &parsed_chars);
        if (parsed_chars != value.size()) {
            throw std::invalid_argument("type:board_id");
        }
        if (parsed <= 0 || parsed > std::numeric_limits<int>::max()) {
            throw std::invalid_argument("value:board_id");
        }

        return static_cast<int>(parsed);
    } catch (const std::invalid_argument&) {
        throw;
    } catch (const std::exception&) {
        throw std::invalid_argument("type:board_id");
    }
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

auto handleGet(const http::request<http::string_body>& req, ConnectionPool& pool,
               int user_id) -> http::response<http::string_body> {
    if (req.method() != http::verb::get) {
        return server::utils::build_error_response(req, http::status::method_not_allowed,
                                                   "DUPLICATE_RESOURCE", "Method not allowed");
    }

    try {
        BoardRepository board_repository(pool);

        const std::optional<int> requested_board_id = parse_board_id_query(req);
        std::optional<Board> board = requested_board_id.has_value()
                                         ? board_repository.find_by_id(*requested_board_id)
                                         : board_repository.find_by_user_id(user_id);

        if (!board.has_value()) {
            return server::utils::build_error_response(req, http::status::not_found,
                                                       "BOARD_NOT_FOUND", "Board not found");
        }

        if (board->user_id_ != user_id) {
            return server::utils::build_error_response(req, http::status::forbidden,
                                                       "RESOURCE_NOT_OWNED",
                                                       "Resource belongs to another user");
        }

        return server::utils::build_json_response(req, http::status::ok,
                                                  json{{"data", model_to_json(*board)}});

    } catch (const std::invalid_argument& e) {
        const std::string message = e.what();
        if (message.rfind("type:", 0) == 0) {
            return server::utils::build_error_response(
                req, http::status::bad_request, "INVALID_FORMAT", "Invalid field format",
                json{{"board_id", "Invalid board_id format"}});
        }
        if (message.rfind("value:", 0) == 0) {
            return server::utils::build_error_response(
                req, http::status::bad_request, "VALIDATION_ERROR", "Validation failed",
                json{{"board_id", "Board id must be positive"}});
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
