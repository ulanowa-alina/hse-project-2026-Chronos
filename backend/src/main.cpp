#include "db/connection_pool.hpp"
#include "db/db_config.hpp"
#include "server/server.hpp"

#include <boost/asio.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <vector>

int main(int argc, char* argv[]) {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink =
        std::make_shared<spdlog::sinks::basic_file_sink_mt>("/app/backend/logs/server.log", true);

    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] [thread: %t] %v");
    console_sink->set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] [thread: %t] %v");

    std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};

    auto logger = std::make_shared<spdlog::logger>("chronos_logger", sinks.begin(), sinks.end());

    spdlog::set_default_logger(logger);

    logger->set_level(spdlog::level::info);
    spdlog::flush_on(spdlog::level::info);

    auto db = load_db_config_from_env();
    ConnectionPool pool(db.connection_info(), db.pool_size);

    if (argc != 3) {
        spdlog::critical("Usage: {} <host> <port>", argv[0]);
        spdlog::critical("Example: {} 0.0.0.0 8080", argv[0]);
        return 1;
    }

    try {
        const std::string host = argv[1];
        const auto port = static_cast<unsigned short>(std::stoi(argv[2]));
        const int threadCount = static_cast<int>(std::thread::hardware_concurrency());

        asio::io_context ioc{threadCount};
        Server server(ioc, host, port, pool);

        spdlog::info("Server started on http://{}:{}", host, port);

        std::vector<std::thread> threads;
        threads.reserve(static_cast<std::size_t>(threadCount - 1));
        for (int idx = 0; idx < threadCount - 1; ++idx) {
            threads.emplace_back([&ioc] { ioc.run(); });
        }
        ioc.run();

        for (auto& thread : threads) {
            thread.join();
        }
    } catch (const std::exception& exc) {
        spdlog::critical("Fatal error: {}", exc.what());
        return 1;
    }
    return 0;
}
