#ifndef POMODORO_SESSION_REPOSITORY_HPP
#define POMODORO_SESSION_REPOSITORY_HPP

#include "../models/pomodoro_session.hpp"
#include "../src/db/connection_pool.hpp"

#include <optional>
#include <pqxx/pqxx>
#include <string>
#include <vector>

class PomodoroSessionRepository {
  public:
    explicit PomodoroSessionRepository(ConnectionPool& pool);

    PomodoroSession save(const PomodoroSession& session);
    std::optional<PomodoroSession> find_by_id(int session_id);
    std::vector<PomodoroSession> find_by_user_id(int user_id);

  private:
    ConnectionPool& pool_;

    PomodoroSession insert(const PomodoroSession& session);
    void update(const PomodoroSession& session);
};

#endif // POMODORO_SESSION_REPOSITORY_HPP
