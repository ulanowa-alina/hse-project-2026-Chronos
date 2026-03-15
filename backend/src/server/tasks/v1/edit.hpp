#ifndef TASKS_V1_EDIT_HPP
#define TASKS_V1_EDIT_HPP

#include <boost/beast/http.hpp>

//TODO: расскоментить, когда замержим
// #include "../../repositories/task_repository.hpp"

namespace http = boost::beast::http;

//TODO: убрать после мержа
class TaskRepository;

namespace tasks::v1 {

auto handleEdit(const http::request<http::string_body>& req, TaskRepository* repository)
    -> http::response<http::string_body>;

} // namespace tasks::v1

#endif // TASKS_V1_EDIT_HPP
