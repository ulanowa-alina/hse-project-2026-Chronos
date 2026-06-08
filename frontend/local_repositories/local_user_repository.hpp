#ifndef LOCAL_USER_REPOSITORY_HPP
#define LOCAL_USER_REPOSITORY_HPP

#include "../local_models/local_user.hpp"

#include <QSqlDatabase>
#include <optional>
#include <vector>

class LocalUserRepository {
  public:
    explicit LocalUserRepository(QSqlDatabase& db);

    LocalUser save(const LocalUser& user);
    std::optional<LocalUser> findById(int user_id);
    std::optional<LocalUser> getCurrentUser();
    std::vector<LocalUser> findUnsynced();
    void markSynced(int user_id);

  private:
    QSqlDatabase& db_;

    LocalUser insert(const LocalUser& user);
    LocalUser update(const LocalUser& user);
};

#endif // LOCAL_USER_REPOSITORY_HPP
