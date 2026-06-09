#include "connection_pool.hpp"

#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

ConnectionPool::ConnectionPool(std::string connection_info, std::size_t pool_size)
    : connection_info_(std::move(connection_info))
    , max_size_(pool_size) {
    if (max_size_ == 0) {
        throw std::invalid_argument("pool_size must be > 0");
    }

    auto c = create_connection();
    free_.push(std::move(c));
    total_created_ = 1;
}

std::unique_ptr<pqxx::connection> ConnectionPool::create_connection() {
    std::unique_ptr<pqxx::connection> c;

    const int max_attempts = 30;
    std::string last_error;

    for (int attempt = 1; attempt <= max_attempts; ++attempt) {
        try {
            c = std::make_unique<pqxx::connection>(connection_info_);
            if (c->is_open()) {
                return c;
            }
        } catch (const pqxx::broken_connection& e) {
            last_error = e.what();
        } catch (const std::exception& e) {
            last_error = e.what();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::string msg = "failed to connect to postgres after retries";
    if (!last_error.empty()) {
        msg += ": ";
        msg += last_error;
    }
    throw std::runtime_error(msg);
}

ConnectionPool::Handle ConnectionPool::acquire() {
    std::unique_lock<std::mutex> lock(m_);

    while (true) {
        if (!free_.empty()) {
            auto c = std::move(free_.front());
            free_.pop();
            return Handle(this, std::move(c));
        }

        if (total_created_ < max_size_) {
            ++total_created_;
            lock.unlock();

            try {
                auto c = create_connection();
                return Handle(this, std::move(c));
            } catch (...) {
                lock.lock();
                --total_created_;
                cv_.notify_one();
                throw;
            }
        }

        cv_.wait(lock);
    }
}

void ConnectionPool::release(std::unique_ptr<pqxx::connection> c) {
    {
        std::lock_guard<std::mutex> lock(m_);

        if (c && c->is_open()) {
            free_.push(std::move(c));
        } else if (total_created_ > 0) {
            --total_created_;
        }
    }

    cv_.notify_one();
}

ConnectionPool::Handle::Handle(ConnectionPool* pool, std::unique_ptr<pqxx::connection> c)
    : pool_(pool)
    , c_(std::move(c)) {
}

ConnectionPool::Handle::Handle(Handle&& other) noexcept
    : pool_(other.pool_)
    , c_(std::move(other.c_)) {
    other.pool_ = nullptr;
}

ConnectionPool::Handle& ConnectionPool::Handle::operator=(Handle&& other) noexcept {
    if (this == &other)
        return *this;

    if (pool_ && c_) {
        pool_->release(std::move(c_));
    }

    pool_ = other.pool_;
    c_ = std::move(other.c_);
    other.pool_ = nullptr;

    return *this;
}

ConnectionPool::Handle::~Handle() {
    if (pool_ && c_) {
        pool_->release(std::move(c_));
    }
}

pqxx::connection& ConnectionPool::Handle::conn() {
    return *c_;
}

pqxx::connection* ConnectionPool::Handle::operator->() {
    return c_.get();
}

ConnectionPool::Handle::operator bool() const noexcept {
    return c_ != nullptr;
}