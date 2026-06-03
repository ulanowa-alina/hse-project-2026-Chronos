#include "get_all.hpp"

#include "../../../../repositories/board_repository.hpp"
#include "../../../../repositories/status_repository.hpp"
#include "../../../../repositories/task_repository.hpp"
#include "../../utils/response_utils.hpp"

#include <array>
#include <boost/url.hpp>
#include <ctime>
#include <limits>
#include <nlohmann/json.hpp>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>

using json = nlohmann::json;

namespace task::v1 {

namespace {

std::string time_to_string_iso8601(std::time_t t) {
    std::array<char, 25> buffer{};
    if (std::strftime(buffer.data(), buffer.size(), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t))) {
        return {buffer.data()};
    }
    throw std::runtime_error("Failed to format timestamp");
}

json model_to_json(const Task& task) {
    json data = {
        {"id", task.id_},
        {"board_id", task.board_id_},
        {"title", task.title_},
        {"description", task.description_},
        {"status_id", task.status_id_},
        {"priority_color", task.priority_color_},
        {"created_at", time_to_string_iso8601(task.created_at_)},
        {"updated_at", time_to_string_iso8601(task.updated_at_)},
    };

    if (task.deadline_) {
        data["deadline"] = time_to_string_iso8601(*task.deadline_);
    } else {
        data["deadline"] = nullptr;
    }

    return data;
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
    } catch (const std::invalid_argument&) {
        throw;
    } catch (const std::exception&) {
        throw std::invalid_argument("type:" + key);
    }
}

} // namespace

auto handleGetAll(const http::request<http::string_body>& req, ConnectionPool& pool,
                  int user_id) -> http::response<http::string_body> {
    spdlog::info("Task get all request received");

    if (req.method() != http::verb::get) {
        spdlog::error("Task get all rejected: method not allowed");
        return server::utils::build_error_response(req, http::status::method_not_allowed,
                                                   "METHOD_NOT_ALLOWED",
                                                   "Only GET is supported for this endpoint");
    }

    try {
        const auto url_view_result = boost::urls::parse_origin_form(req.target());
        if (!url_view_result) {
            spdlog::error("Task get all rejected: Invalid field format");
            return server::utils::build_error_response(req, http::status::bad_request,
                                                       "INVALID_FORMAT", "Invalid field format",
                                                       json{{"query", "Invalid query format"}});
        }

        const auto params = url_view_result->params();

        bool has_board_id = false;
        bool has_status_id = false;

        const std::optional<int> board_id = parse_int_param(params, "board_id", has_board_id);
        const std::optional<int> status_id = parse_int_param(params, "status_id", has_status_id);

        if (has_status_id && !has_board_id) {
            spdlog::error("Task get all rejected: invalid status_id");
            return server::utils::build_error_response(
                req, http::status::bad_request, "VALIDATION_ERROR", "Invalid field value",
                json{{"status_id", "status_id can be used only with board_id"}});
        }

        TaskRepository task_repository(pool);
        std::vector<Task> tasks;

        if (board_id.has_value()) {
            spdlog::info("Task get all for board request received");
            BoardRepository board_repository(pool);
            const std::optional<Board> board = board_repository.find_by_id(*board_id);
            if (!board.has_value()) {
                spdlog::error("Task get all rejected: board with id={} not found",
                              board_id.value());
                return server::utils::build_error_response(req, http::status::not_found,
                                                           "BOARD_NOT_FOUND", "Board not found");
            }

            if (board->user_id_ != user_id) {
                spdlog::error("Task get all rejected: board with id={} belongs to another user",
                              board->id_);
                return server::utils::build_error_response(req, http::status::forbidden,
                                                           "RESOURCE_NOT_OWNED",
                                                           "Resource belongs to another user");
            }

            if (status_id.has_value()) {
                spdlog::info("Task get all for status request received");
                StatusRepository status_repository(pool);
                const std::optional<Status> status = status_repository.find_by_id(*status_id);
                if (!status.has_value()) {
                    spdlog::error("Task get all rejected: status with id={} not found",
                                  status_id.value());
                    return server::utils::build_error_response(
                        req, http::status::not_found, "STATUS_NOT_FOUND", "Status not found");
                }

                if (status->board_id_ != *board_id) {
                    spdlog::error("Task get all rejected: invalid status_id");
                    return server::utils::build_error_response(
                        req, http::status::bad_request, "VALIDATION_ERROR", "Invalid field value",
                        json{{"status_id", "status_id must reference a status from this board"}});
                }

                tasks = task_repository.find_by_board_and_status_id(*board_id, *status_id);
            } else {
                tasks = task_repository.find_by_board_id(*board_id);
            }
        } else {
            spdlog::info("Task get all for user request received");
            tasks = task_repository.find_by_user_id(user_id);
        }

        json data = json::array();
        for (const auto& task : tasks) {
            data.push_back(model_to_json(task));
        }

        spdlog::info("Task get all succeeded with task_count={}", tasks.size());
        return server::utils::build_json_response(req, http::status::ok, json{{"data", data}});
    } catch (const std::invalid_argument& e) {
        const std::string message = e.what();

        if (message.rfind("type:", 0) == 0) {
            spdlog::error("Task get all rejected: Invalid field format");
            const std::string field = message.substr(5);
            return server::utils::build_error_response(
                req, http::status::bad_request, "INVALID_FORMAT", "Invalid field format",
                json{{field, "Field " + field + " has invalid type"}});
        }

        if (message.rfind("value:", 0) == 0) {
            spdlog::error("Task get all rejected: Invalid field value");
            const std::string field = message.substr(6);
            return server::utils::build_error_response(
                req, http::status::bad_request, "VALIDATION_ERROR", "Invalid field value",
                json{{field, "Field " + field + " must be a positive integer"}});
        }

        spdlog::error("Task get all rejected: validation error");
        return server::utils::build_error_response(req, http::status::bad_request,
                                                   "VALIDATION_ERROR", message);
    } catch (const std::runtime_error& e) {
        spdlog::error("Task get all failed with database error: {}", e.what());
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "DATABASE_ERROR", e.what());
    } catch (const std::exception& e) {
        spdlog::error("Task get all failed with unexpected error: {}", e.what());
        return server::utils::build_error_response(req, http::status::internal_server_error,
                                                   "INTERNAL_ERROR", e.what());
    }
}

} // namespace task::v1
