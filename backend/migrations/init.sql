CREATE TABLE users (
                       id SERIAL PRIMARY KEY,
                       email VARCHAR(255) NOT NULL UNIQUE,
                       name VARCHAR(255) NOT NULL,
                       password_hash VARCHAR(255) NOT NULL,
                       created_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE boards (
                        id SERIAL PRIMARY KEY,
                        user_id INT NOT NULL,
                        title VARCHAR(255) NOT NULL,
                        created_at TIMESTAMP DEFAULT NOW(),
                        CONSTRAINT fk_user FOREIGN KEY (user_id) REFERENCES users(id)
);

CREATE TABLE tasks (
                       id SERIAL PRIMARY KEY,
                       board_id INT NOT NULL,
                       title VARCHAR(255) NOT NULL,
                       description TEXT,
                       deadline TIMESTAMP,
                       status VARCHAR(50) NOT NULL DEFAULT 'todo',
                       priority INT NOT NULL DEFAULT 0,
                       created_at TIMESTAMP DEFAULT NOW(),
                       updated_at TIMESTAMP DEFAULT NOW(),
                       CONSTRAINT fk_board FOREIGN KEY (board_id) REFERENCES boards(id)
);