PRAGMA foreign_keys = ON;

CREATE TABLE users(
    id INTEGER PRIMARY KEY,
    email TEXT NOT NULL UNIQUE CHECK (email <> ''),
    name TEXT NOT NULL CHECK (length(name) BETWEEN 1 AND 50),
    status TEXT NOT NULL CHECK (length(status) BETWEEN 1 AND 255),
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    is_sync INTEGER NOT NULL DEFAULT 1,
    is_deleted INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE boards (
                        id INTEGER PRIMARY KEY,
                        title TEXT NOT NULL CHECK (length(title) BETWEEN 1 AND 100),
                        description TEXT CHECK (description IS NULL OR length(description) <= 1000),
                        is_private INTEGER NOT NULL DEFAULT 0,
                        created_at TEXT NOT NULL DEFAULT (datetime('now')),
                        updated_at TEXT NOT NULL DEFAULT (datetime('now')),
                        is_sync INTEGER NOT NULL DEFAULT 1,
                        is_deleted INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE statuses (
                          id INTEGER PRIMARY KEY,
                          board_id INTEGER NOT NULL REFERENCES boards(id) ON DELETE CASCADE,
                          name TEXT NOT NULL CHECK (length(name) BETWEEN 1 AND 50),
                          position INTEGER NOT NULL DEFAULT 0 CHECK (position >= 0),
                          is_sync INTEGER NOT NULL DEFAULT 1,
                          is_deleted INTEGER NOT NULL DEFAULT 0,
                          UNIQUE (board_id, name)
);

CREATE TABLE tasks (
                       id INTEGER PRIMARY KEY,
                       board_id INTEGER NOT NULL,
                       title TEXT NOT NULL CHECK (length(title) BETWEEN 1 AND 100),
                       description TEXT CHECK (description IS NULL OR length(description) <= 1000),
                       status_id INTEGER NOT NULL,
                       priority_color TEXT NOT NULL CHECK (length(priority_color) BETWEEN 1 AND 50),
                       deadline TEXT,
                       created_at TEXT NOT NULL DEFAULT (datetime('now')),
                       updated_at TEXT NOT NULL DEFAULT (datetime('now')),
                       is_sync INTEGER NOT NULL DEFAULT 1,
                       is_deleted INTEGER NOT NULL DEFAULT 0,
                       CONSTRAINT fk_board FOREIGN KEY (board_id) REFERENCES boards(id) ON DELETE CASCADE,
                       CONSTRAINT fk_status FOREIGN KEY (status_id) REFERENCES statuses(id) ON DELETE CASCADE
);
