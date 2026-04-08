CREATE TABLE users (
                       id SERIAL PRIMARY KEY,
                       email VARCHAR(255) NOT NULL UNIQUE CHECK (email <> ''),
                       name VARCHAR(255) NOT NULL CHECK (char_length(name) BETWEEN 1 AND 50),
                       status VARCHAR(255) NOT NULL CHECK (char_length(status) BETWEEN 1 AND 255),
                       password_hash VARCHAR(255) NOT NULL CHECK (password_hash <> ''),
                       created_at TIMESTAMP NOT NULL DEFAULT NOW()
);

CREATE TABLE boards (
                        id SERIAL PRIMARY KEY,
                        user_id INT NOT NULL,
                        title VARCHAR(255) NOT NULL CHECK (char_length(title) BETWEEN 1 AND 100),
                        description TEXT CHECK (description IS NULL OR char_length(description) <= 1000),
                        is_private BOOLEAN NOT NULL DEFAULT FALSE,
                        created_at TIMESTAMP NOT NULL DEFAULT NOW(),
                        updated_at TIMESTAMP NOT NULL DEFAULT NOW(),
                        CONSTRAINT fk_user FOREIGN KEY (user_id) REFERENCES users(id)
);

CREATE TABLE statuses (
                          id SERIAL PRIMARY KEY,
                          board_id INT NOT NULL REFERENCES boards(id) ON DELETE CASCADE,
                          name VARCHAR(50) NOT NULL CHECK (char_length(name) BETWEEN 1 AND 50),
                          position INT NOT NULL DEFAULT 0 CHECK (position >= 0),
                          UNIQUE (board_id, name)
);

CREATE TABLE tasks (
                       id SERIAL PRIMARY KEY,
                       board_id INT NOT NULL,
                       title VARCHAR(255) NOT NULL CHECK (char_length(title) BETWEEN 1 AND 100),
                       description TEXT CHECK (description IS NULL OR char_length(description) <= 1000),
                       status_id INT NOT NULL,
                       priority_color VARCHAR(50) NOT NULL CHECK (char_length(priority_color) BETWEEN 1 AND 50),
                       deadline TIMESTAMP,
                       created_at TIMESTAMP NOT NULL DEFAULT NOW(),
                       updated_at TIMESTAMP NOT NULL DEFAULT NOW(),
                       CONSTRAINT fk_board FOREIGN KEY (board_id) REFERENCES boards(id) ON DELETE CASCADE,
                       CONSTRAINT fk_status FOREIGN KEY (status_id) REFERENCES statuses(id) ON DELETE CASCADE
);
