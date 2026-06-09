#include "pomodoro_session_repository.hpp"

#include <optional>

PomodoroSessionRepository::PomodoroSessionRepository(ConnectionPool& pool)
    : pool_(pool) {
}

PomodoroSession PomodoroSessionRepository::insert(const PomodoroSession& session) {
    auto handle = pool_.acquire();
    pqxx::work txn(handle.conn());

    pqxx::result r = txn.exec_params(
        "INSERT INTO pomodoro_sessions (user_id, goal_minutes, work_duration_seconds, "
        "break_duration_seconds, completed_cycles, started_at) "
        "VALUES ($1, NULLIF($2, 0), $3, $4, $5, TO_TIMESTAMP($6)) "
        "RETURNING id, EXTRACT(EPOCH FROM started_at)::bigint AS started_sec",
        session.user_id_, session.goal_minutes_.has_value() ? session.goal_minutes_.value() : 0,
        session.work_duration_seconds_, session.break_duration_seconds_, session.completed_cycles_,
        static_cast<long>(session.started_at_));

    txn.commit();
    return PomodoroSession(
        r[0][0].as<int>(), session.user_id_, session.goal_minutes_, session.work_duration_seconds_,
        session.break_duration_seconds_, session.completed_cycles_,
        static_cast<std::time_t>(r[0]["started_sec"].as<long>()), session.completed_at_);
}

void PomodoroSessionRepository::update(const PomodoroSession& session) {
    auto handle = pool_.acquire();
    pqxx::work txn(handle.conn());

    std::string completed_at_param =
        session.completed_at_.has_value()
            ? "TO_TIMESTAMP(" + std::to_string(static_cast<long>(session.completed_at_.value())) +
                  ")"
            : "NULL";

    txn.exec_params("UPDATE pomodoro_sessions SET user_id = $1, goal_minutes = NULLIF($2, 0), "
                    "work_duration_seconds = $3, break_duration_seconds = $4, "
                    "completed_cycles = $5, completed_at = " +
                        completed_at_param +
                        " "
                        "WHERE id = $6",
                    session.user_id_,
                    session.goal_minutes_.has_value() ? session.goal_minutes_.value() : 0,
                    session.work_duration_seconds_, session.break_duration_seconds_,
                    session.completed_cycles_, session.id_);

    txn.commit();
}

PomodoroSession PomodoroSessionRepository::save(const PomodoroSession& session) {
    if (session.id_ == 0) {
        return insert(session);
    }
    update(session);
    return session;
}

std::optional<PomodoroSession> PomodoroSessionRepository::find_by_id(int session_id) {
    auto handle = pool_.acquire();
    pqxx::work txn(handle.conn());

    pqxx::result r = txn.exec_params(
        "SELECT id, user_id, goal_minutes, work_duration_seconds, break_duration_seconds, "
        "completed_cycles, EXTRACT(EPOCH FROM started_at)::bigint AS started_sec, "
        "EXTRACT(EPOCH FROM completed_at)::bigint AS completed_sec "
        "FROM pomodoro_sessions WHERE id = $1",
        session_id);

    if (r.empty()) {
        return std::nullopt;
    }

    txn.commit();
    const auto& row = r[0];

    std::optional<int> goal_minutes;
    if (!row["goal_minutes"].is_null()) {
        goal_minutes = row["goal_minutes"].as<int>();
    }

    std::optional<std::time_t> completed_at;
    if (!row["completed_sec"].is_null()) {
        completed_at = static_cast<std::time_t>(row["completed_sec"].as<long>());
    }

    return PomodoroSession(row["id"].as<int>(), row["user_id"].as<int>(), goal_minutes,
                           row["work_duration_seconds"].as<int>(),
                           row["break_duration_seconds"].as<int>(),
                           row["completed_cycles"].as<int>(),
                           static_cast<std::time_t>(row["started_sec"].as<long>()), completed_at);
}

std::vector<PomodoroSession> PomodoroSessionRepository::find_by_user_id(int user_id) {
    auto handle = pool_.acquire();
    pqxx::work txn(handle.conn());

    pqxx::result r = txn.exec_params(
        "SELECT id, user_id, goal_minutes, work_duration_seconds, break_duration_seconds, "
        "completed_cycles, EXTRACT(EPOCH FROM started_at)::bigint AS started_sec, "
        "EXTRACT(EPOCH FROM completed_at)::bigint AS completed_sec "
        "FROM pomodoro_sessions WHERE user_id = $1 ORDER BY started_at DESC",
        user_id);

    std::vector<PomodoroSession> sessions;
    for (const auto& row : r) {
        std::optional<int> goal_minutes;
        if (!row["goal_minutes"].is_null()) {
            goal_minutes = row["goal_minutes"].as<int>();
        }

        std::optional<std::time_t> completed_at;
        if (!row["completed_sec"].is_null()) {
            completed_at = static_cast<std::time_t>(row["completed_sec"].as<long>());
        }

        sessions.emplace_back(
            row["id"].as<int>(), row["user_id"].as<int>(), goal_minutes,
            row["work_duration_seconds"].as<int>(), row["break_duration_seconds"].as<int>(),
            row["completed_cycles"].as<int>(),
            static_cast<std::time_t>(row["started_sec"].as<long>()), completed_at);
    }

    txn.commit();
    return sessions;
}
