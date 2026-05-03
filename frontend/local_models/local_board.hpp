#ifndef LOCAL_BOARD_HPP
#define LOCAL_BOARD_HPP

#include <QString>

struct LocalBoard {
    int id_;
    QString title_;
    QString description_;
    int is_private_;
    QString created_at_;
    QString updated_at_;
    int is_sync_;
    int is_deleted_;

    LocalBoard() = default;

    LocalBoard(int id, const QString& title, const QString desc, int is_private,
               const QString& created_at, const QString& updated_at_, int is_sync, int is_deleted)
        : id_(id)
        , title_(title)
        , description_(desc)
        , is_private_(is_private)
        , created_at_(created_at)
        , updated_at_(updated_at_)
        , is_sync_(is_sync)
        , is_deleted_(is_deleted) {
    }
};
#endif // LOCAL_BOARD_HPP
