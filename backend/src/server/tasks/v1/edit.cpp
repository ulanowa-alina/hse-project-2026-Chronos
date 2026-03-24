#include "edit.hpp"

#include <ctime>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace tasks::v1 {

std::string time_to_string_iso8601(std::time_t t) {
    std::array<char, 25> buffer{};
    if (std::strftime(buffer.data(), buffer.size(), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t))) {
        return {buffer.data()};
    }
    return "";
}

auto handleEdit(
    const http::request<http::string_body>& req,
    TaskRepository* repository
    ) -> http::response<http::string_body> {

    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::access_control_allow_origin, "*");

    if (repository == nullptr) {
        res.result(http::status::internal_server_error);
        res.body() = R"({"error": "Repository not initialized"})";
        res.prepare_payload();
        return res;
    }

    try {
        auto body = json::parse(req.body());

        if (!body.contains("task_id")) {
            res.result(http::status::bad_request);
            res.body() =
                R"({"error": {"code": "MISSING_FIELD", "message": "task_id is required"}})";
            res.prepare_payload();
            return res;
        }

        int task_id = body["task_id"].get<int>();

        auto task_opt = repository->find_by_id(task_id);
        if (!task_opt) {
            res.result(http::status::not_found);
            res.body() = R"({"error": {"code": "NOT_FOUND", "message": "Task not found"}})";
            res.prepare_payload();
            return res;
        }

        Task task = task_opt.value();

        if (body.contains("title")) {
            task.title_ = body["title"].get<std::string>();
        }
        if (body.contains("text")) {
            task.description_ = body["text"].get<std::string>();
        }
        if (body.contains("status_id")) {
            task.status_id_ = body["status_id"].get<int>();
        }
        if (body.contains("priority")) {
            task.priority_ = body["priority"].get<int>();
        }

        task.updated_at_ = std::time(nullptr);
        Task updated_task = repository->save(task);

        json responseData = {{"data",
                              {{"id", updated_task.id_},
                               {"boardId", updated_task.board_id_},
                               {"title", updated_task.title_},
                               {"description", updated_task.description_},
                               {"status", updated_task.status_id_},
                               {"priority", updated_task.priority_},
                               {"date", time_to_string_iso8601(updated_task.updated_at_)}}}};
        res.body() = responseData.dump();
    } catch (const std::invalid_argument& e) {

        res.result(http::status::bad_request);
        res.body() =
            json::object(
                {{"error", json::object({{"code", "VALIDATION_ERROR"}, {"message", e.what()}})}})
                .dump();
    } catch (const std::exception& e) {
        res.result(http::status::internal_server_error);
        res.body() =
            R"({"error": {"code": "INTERNAL_ERROR", "message": "JSON Parse Error or Branch Sync Issue"}})";
    }

    res.prepare_payload();
    return res;
}

} // namespace tasks::v1
