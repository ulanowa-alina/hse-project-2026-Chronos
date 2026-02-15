#ifndef BOARD_HPP
#define BOARD_HPP

#include <pqxx/pqxx>
#include <string>

class Board {
  public:
    int id;
    int user_id;
    std::string title;

    Board() = default;
    Board(int _id, int _user_id, const std::string& _title)
        : id(_id)
        , user_id(_user_id)
        , title(_title) {
    }

    void save(pqxx::connection& conn);
    static Board find_by_id(pqxx::connection& conn, int id);
};

#endif // BOARD_HPP