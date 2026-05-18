#ifndef LOCAL_STATUS_HPP
#define LOCAL_STATUS_HPP

#include <QString>

struct LocalStatus {
    int id_;
    int board_id_;
    QString name_;
    int position_;
    int is_sync_;
    int is_deleted_;
    int is_new_;

    LocalStatus() = default;

    LocalStatus(int id, int board_id, const QString& name, int position, int is_sync,
                int is_deleted, int is_new)
        : id_(id)
        , board_id_(board_id)
        , name_(name)
        , position_(position)
        , is_sync_(is_sync)
        , is_deleted_(is_deleted)
        , is_new_(is_new) {
    }
};
#endif // LOCAL_STATUS_HPP
