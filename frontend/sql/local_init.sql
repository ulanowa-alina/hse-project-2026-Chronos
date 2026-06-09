PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS users(
    id INTEGER PRIMARY KEY,
    email TEXT NOT NULL UNIQUE CHECK (email <> ''),
    name TEXT NOT NULL CHECK (length(name) BETWEEN 1 AND 50),
    status TEXT NOT NULL CHECK (length(status) BETWEEN 1 AND 255),
    password_hash TEXT,

    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    deleted_at TEXT,
    sync_status TEXT NOT NULL DEFAULT 'pending'
        CHECK (sync_status IN ('pending', 'synced', 'conflict')),
    server_version INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS boards (
                        id INTEGER PRIMARY KEY,
                        user_id INTEGER NOT NULL,
                        title TEXT NOT NULL CHECK (length(title) BETWEEN 1 AND 100),
                        description TEXT CHECK (description IS NULL OR length(description) <= 1000),

                        created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                        updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                        deleted_at TEXT,
                        sync_status TEXT NOT NULL DEFAULT 'pending'
                            CHECK (sync_status IN ('pending', 'synced', 'conflict')),
                        server_version INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS statuses (
                          id INTEGER PRIMARY KEY,
                          board_id INTEGER NOT NULL REFERENCES boards(id) ON DELETE CASCADE,
                          name TEXT NOT NULL CHECK (length(name) BETWEEN 1 AND 50),
                          position INTEGER NOT NULL DEFAULT 0 CHECK (position >= 0),

                          created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                          updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                          deleted_at TEXT,
                          sync_status TEXT NOT NULL DEFAULT 'pending'
                              CHECK (sync_status IN ('pending', 'synced', 'conflict')),
                          server_version INTEGER NOT NULL DEFAULT 0,

                          UNIQUE (board_id, name)
);

CREATE TABLE IF NOT EXISTS tasks (
                       id INTEGER PRIMARY KEY,
                       board_id INTEGER NOT NULL,
                       title TEXT NOT NULL CHECK (length(title) BETWEEN 1 AND 100),
                       description TEXT CHECK (description IS NULL OR length(description) <= 1000),
                       status_id INTEGER NOT NULL,
                       priority_color TEXT NOT NULL CHECK (length(priority_color) BETWEEN 1 AND 50),
                       deadline TEXT,
                       is_completed INTEGER NOT NULL DEFAULT 0,

                       created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                       updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                       deleted_at TEXT,
                       sync_status TEXT NOT NULL DEFAULT 'pending'
                           CHECK (sync_status IN ('pending', 'synced', 'conflict')),
                       server_version INTEGER NOT NULL DEFAULT 0,
                       CONSTRAINT fk_board FOREIGN KEY (board_id) REFERENCES boards(id) ON DELETE CASCADE,
                       CONSTRAINT fk_status FOREIGN KEY (status_id) REFERENCES statuses(id) ON DELETE CASCADE
);

CREATE TRIGGER IF NOT EXISTS trg_users_updated_at
AFTER UPDATE ON users
                            FOR EACH ROW
                            WHEN NEW.updated_at = OLD.updated_at
BEGIN
UPDATE users
SET updated_at = CURRENT_TIMESTAMP
WHERE id = NEW.id;
END;

CREATE TRIGGER IF NOT EXISTS trg_boards_updated_at
AFTER UPDATE ON boards
                            FOR EACH ROW
                            WHEN NEW.updated_at = OLD.updated_at
BEGIN
UPDATE boards
SET updated_at = CURRENT_TIMESTAMP
WHERE id = NEW.id;
END;

CREATE TRIGGER IF NOT EXISTS trg_statuses_updated_at
AFTER UPDATE ON statuses
                            FOR EACH ROW
                            WHEN NEW.updated_at = OLD.updated_at
BEGIN
UPDATE statuses
SET updated_at = CURRENT_TIMESTAMP
WHERE id = NEW.id;
END;

CREATE TRIGGER IF NOT EXISTS trg_tasks_updated_at
AFTER UPDATE ON tasks
                            FOR EACH ROW
                            WHEN NEW.updated_at = OLD.updated_at
BEGIN
UPDATE tasks
SET updated_at = CURRENT_TIMESTAMP
WHERE id = NEW.id;
END;

CREATE TABLE IF NOT EXISTS pomodoro_sessions (
                           id INTEGER PRIMARY KEY,
                           user_id INTEGER NOT NULL,
                           goal_minutes INTEGER,
                           work_duration_seconds INTEGER NOT NULL,
                           break_duration_seconds INTEGER NOT NULL,
                           completed_cycles INTEGER NOT NULL DEFAULT 0,
                           started_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                           completed_at TEXT,

                           created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                           updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                           deleted_at TEXT,
                           sync_status TEXT NOT NULL DEFAULT 'pending'
                               CHECK (sync_status IN ('pending', 'synced', 'conflict')),
                           server_version INTEGER NOT NULL DEFAULT 0
);

CREATE TRIGGER IF NOT EXISTS trg_pomodoro_sessions_updated_at
AFTER UPDATE ON pomodoro_sessions
                            FOR EACH ROW
                            WHEN NEW.updated_at = OLD.updated_at
BEGIN
UPDATE pomodoro_sessions
SET updated_at = CURRENT_TIMESTAMP
WHERE id = NEW.id;
END;
