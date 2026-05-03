#ifndef LOCAL_BOARD_REPOSITORY_HPP
#define LOCAL_BOARD_REPOSITORY_HPP

#include "../local_models/local_board.hpp"

#include <QSqlDatabase>
#include <optional>
#include <vector>

class LocalBoardRepository {
  public:
    explicit LocalBoardRepository(QSqlDatabase& db);

    LocalBoard save(const LocalBoard& board);
    std::optional<LocalBoard> findById(int board_id);
    std::vector<LocalBoard> findAll(int user_id);
    std::vector<LocalBoard> findUnsynced();
    void deleteById(int board_id);
    void markSynced(int board_id);

  private:
    QSqlDatabase& db_;

    LocalBoard insert(const LocalBoard& board);
    LocalBoard update(const LocalBoard& board);
};

#endif // LOCAL_BOARD_REPOSITORY_HPP
