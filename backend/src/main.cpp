#include "db/connection_pool.hpp"
#include "db/db_config.hpp"
#include "server/server.hpp"

#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

int main(int argc, char* argv[]) {
    auto db = load_db_config_from_env();
    ConnectionPool pool(db.connection_info(), db.pool_size);

    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <host> <port>\n";
        std::cerr << "Example: " << argv[0] << " 0.0.0.0 8080\n";
        return 1;
    }

    try {
        const std::string host = argv[1];
        const auto port = static_cast<unsigned short>(std::stoi(argv[2]));
        const int threadCount = static_cast<int>(std::thread::hardware_concurrency());

        asio::io_context ioc{threadCount};
        Server server(ioc, host, port, pool);

        std::cout << "Server started on http://" << host << ":" << port << "\n";

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
        std::cerr << "Fatal error: " << exc.what() << "\n";
        return 1;
    }
    return 0;
}
