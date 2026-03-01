#include <pqxx/pqxx>
#include <iostream>

int main() {
    try {
        pqxx::connection conn(
            "host=localhost port=5432 user=postgres password=mysecretpassword dbname=chronos_db");

        if (!conn.is_open()) {
            std::cerr << "Failed to open database connection\n";
            return 1;
        }

        std::cout << "Connected to database: " << conn.dbname() << '\n';
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }
}
