#include "connection_pool.hpp"

#include <stdexcept>
#include <utility>
#include <chrono>
#include <thread>

ConnectionPool::ConnectionPool(std::string connection_info, std::size_t pool_size)
    : connection_info_(std::move(connection_info)) {
    if (pool_size == 0) {
        throw std::invalid_argument("pool_size must be > 0");
    }

    for (std::size_t i = 0; i < pool_size; ++i) {
        std::unique_ptr<pqxx::connection> c;

        const int max_attempts = 30;
        for (int attempt = 1; attempt <= max_attempts; ++attempt) {
            try {
                c = std::make_unique<pqxx::connection>(connection_info_);
                if (c->is_open()) break;
            } catch (const pqxx::broken_connection&) {
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        if (!c || !c->is_open()) {
            throw std::runtime_error("failed to connect to postgres after retries");
        }

        free_.push(std::move(c));
    }
}

ConnectionPool::Handle ConnectionPool::acquire() {
    std::unique_lock<std::mutex> lock(m_);
    cv_.wait(lock, [&] { return !free_.empty(); });

    auto c = std::move(free_.front());
    free_.pop();

    return Handle(this, std::move(c));
}

void ConnectionPool::release(std::unique_ptr<pqxx::connection> c) {
    {
        std::lock_guard<std::mutex> lock(m_);
        free_.push(std::move(c));
    }
    cv_.notify_one();
}

ConnectionPool::Handle::Handle(ConnectionPool* pool, std::unique_ptr<pqxx::connection> c)
    : pool_(pool), c_(std::move(c)) {}

ConnectionPool::Handle::Handle(Handle&& other) noexcept
    : pool_(other.pool_), c_(std::move(other.c_)) {
    other.pool_ = nullptr;
}

ConnectionPool::Handle& ConnectionPool::Handle::operator=(Handle&& other) noexcept {
    if (this == &other) return *this;

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