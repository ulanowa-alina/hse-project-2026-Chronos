#include "edit.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <ctime>

using json = nlohmann::json;

// ------------------ВРЕМЕННЫЙ БЛОК-------------------
// TODO: (Удалить после мержа веток)
struct Task {
    int id_;
    int board_id_;
    std::string title_;
    std::string description_;
    std::time_t deadline_;
    std::string status_;
    std::string priority_;  //TODO: в task.hpp тоже на string поменять
    std::time_t created_at_;
    std::time_t updated_at_;
};

class TaskRepository {};
// --------------------------------------------------

namespace tasks::v1 {

auto handleEdit(const http::request<http::string_body>& req, TaskRepository* repository)
    -> http::response<http::string_body> {

    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::access_control_allow_origin, "*");

    try {
        auto body = json::parse(req.body());

        if (!body.contains("task_id")) {
            res.result(http::status::bad_request);
            res.body() = R"({"error": {"code": "MISSING_FIELD", "message": "task_id is required"}})";
            res.prepare_payload();
            return res;
        }

        Task task;
        task.id_ = body["task_id"].get<int>();
        task.board_id_ = 1;
        task.title_ = "lab06-mytest";
        task.status_ = "todo";
        task.priority_ = "gray";

        if (body.contains("title")) task.title_ = body["title"];
        if (body.contains("text"))  task.description_ = body["text"];
        if (body.contains("status")) task.status_ = body["status"];
        if (body.contains("priorityColor")) task.priority_ = body["priorityColor"];

        task.updated_at_ = std::time(nullptr);

        json responseData = {
            {"data", {
                         {"id", task.id_},
                         {"boardId", task.board_id_},
                         {"title", task.title_},
                         {"description", task.description_},
                         {"status", task.status_},
                         {"priorityColor", task.priority_},
                         {"date", task.updated_at_}
                     }}
        };
        res.body() = responseData.dump();

    } catch (const std::exception& e) {
        res.result(http::status::internal_server_error);
        res.body() = R"({"error": {"code": "INTERNAL_ERROR", "message": "JSON Parse Error or Branch Sync Issue"}})";
    }

    res.prepare_payload();
    return res;
}

} // namespace tasks::v1
