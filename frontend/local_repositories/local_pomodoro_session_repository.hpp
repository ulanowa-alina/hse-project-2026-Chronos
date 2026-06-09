#ifndef LOCAL_POMODORO_SESSION_REPOSITORY_HPP
#define LOCAL_POMODORO_SESSION_REPOSITORY_HPP

#include "../local_models/local_pomodoro_session.hpp"

#include <QSqlDatabase>
#include <optional>
#include <vector>

class LocalPomodoroSessionRepository {
  public:
    explicit LocalPomodoroSessionRepository(QSqlDatabase& db);

    std::optional<LocalPomodoroSession> findById(int id);
    std::vector<LocalPomodoroSession> findByUserId(int user_id);
    std::vector<LocalPomodoroSession> findAll();
    bool insert(const LocalPomodoroSession& session);
    bool update(const LocalPomodoroSession& session);
    bool remove(int id);
    bool deleteById(int id);
    int getNextId();

  private:
    QSqlDatabase& db_;
    LocalPomodoroSession fromQuery(const QSqlQuery& query);
};

#endif // LOCAL_POMODORO_SESSION_REPOSITORY_HPP
