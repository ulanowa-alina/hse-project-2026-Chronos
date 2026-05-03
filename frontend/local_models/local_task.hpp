#ifndef LOCAL_TASK_HPP
#define LOCAL_TASK_HPP

#include <QString>

struct LocalTask {
    int id_;
    int board_id_;
    QString title_;
    QString description_;
    int status_id_;
    QString priority_color_;
    QString deadline_;
    QString created_at_;
    QString updated_at_;
    int is_sync_;
    int is_deleted_;

    LocalTask() = default;

    LocalTask(int id, int board_id, const QString& title, const QString& description, int status_id,
              const QString& priority_color, const QString& deadline, const QString& created_at,
              const QString& updated_at, int is_sync, int is_deleted)
        : id_(id)
        , board_id_(board_id)
        , title_(title)
        , description_(description)
        , status_id_(status_id)
        , priority_color_(priority_color)
        , deadline_(deadline)
        , created_at_(created_at)
        , updated_at_(updated_at)
        , is_sync_(is_sync)
        , is_deleted_(is_deleted) {
    }
};

#endif // LOCAL_TASK_HPP
