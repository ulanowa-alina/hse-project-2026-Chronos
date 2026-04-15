#include "get_all.hpp"

#include "../../../../repositories/board_repository.hpp"
#include "../../utils/response_utils.hpp"

#include <array>
#include <ctime>
#include <nlohmann/json.hpp>
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

auto handleGetAll(const http::request<http::string_body>& req, ConnectionPool& pool,
                  int user_id) -> http::response<http::string_body> {
    if (req.method() != http::verb::get) {
        return server::utils::build_error_response(req, http::status::method_not_allowed,
                                                   "DUPLICATE_RESOURCE", "Method not allowed");
    }

    try {
        BoardRepository board_repository(pool);
        const std::vector<Board> boards = board_repository.find_all_by_user_id(user_id);

        json data = json::array();
        for (const auto& board : boards) {
            data.push_back(model_to_json(board));
        }

        return server::utils::build_json_response(req, http::status::ok, json{{"data", data}});
    } catch (const std::runtime_error&) {
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "DATABASE_ERROR", "Database error");
    } catch (const std::exception&) {
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "INTERNAL_ERROR", "Internal server error");
    }
}

} // namespace board::v1
