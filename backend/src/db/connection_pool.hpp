#pragma once

#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

#include <pqxx/pqxx>

class ConnectionPool {
public:
    ConnectionPool(std::string connection_info, std::size_t pool_size);
    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;

    class Handle {
    public:
        Handle() = default;
        Handle(Handle&& other) noexcept;
        Handle& operator=(Handle&& other) noexcept;
        ~Handle();

        pqxx::connection& conn();
        pqxx::connection* operator->();

        explicit operator bool() const noexcept;

    private:
        friend class ConnectionPool;
        Handle(ConnectionPool* pool, std::unique_ptr<pqxx::connection> c);

        ConnectionPool* pool_ = nullptr;
        std::unique_ptr<pqxx::connection> c_;
    };

    Handle acquire();

private:
    void release(std::unique_ptr<pqxx::connection> c);

    std::string connection_info_;
    std::mutex m_;
    std::condition_variable cv_;
    std::queue<std::unique_ptr<pqxx::connection>> free_;
};