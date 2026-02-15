#ifndef TASK_H
#define TASK_H

#include <ctime>
#include <pqxx/pqxx>
#include <string>

class Task {
  public:
    int id;
    int board_id;
    std::string title;
    std::string description;
    std::time_t deadline;
    std::string status;
    int priority;

    std::time_t created_at;
    std::time_t updated_at;

    Task() = default;
    Task(int _id, int _board_id, const std::string& _title, const std::string& _description,
         std::time_t _deadline, const std::string& _status, int _priority, std::time_t _created_at,
         std::time_t _updated_at)
        : id(_id)
        , board_id(_board_id)
        , title(_title)
        , description(_description)
        , deadline(_deadline)
        , status(_status)
        , priority(_priority)
        , created_at(_created_at)
        , updated_at(_updated_at) {
    }

    void save(pqxx::connection& conn);
    static Task find_by_id(pqxx::connection& conn, int id);
};

#endif // TASK_H