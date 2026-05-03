#ifndef LOCAL_STATUS_REPOSITORY_HPP
#define LOCAL_STATUS_REPOSITORY_HPP

#include "../local_models/local_status.hpp"

#include <QSqlDatabase>
#include <optional>
#include <vector>

class LocalSatusRepository {
  public:
    explicit LocalSatusRepository(QSqlDatabase& db);

    LocalStatus save(const LocalStatus& status);
    std::optional<LocalStatus> findByid(int status_id);
    std::vector<LocalStatus> findByBoardId(int board_id);
    std::vector<LocalStatus> findUnsynced();
    void deleteById(int status_id);
    void markSynced(int status_id);

  private:
    QSqlDatabase& db_;

    LocalStatus insert(const LocalStatus& status);
    LocalStatus update(const LocalStatus& status);
};

#endif // LOCAL_STATUS_REPOSITORY_HPP
