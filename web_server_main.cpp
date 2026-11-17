#include "WebApplication.h"

#include <charconv>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

int parsePort(const std::string& value) {
    int port = 0;
    const char* begin = value.data();
    const char* end = begin + value.size();
    const auto result = std::from_chars(begin, end, port);
    if (result.ec != std::errc() || result.ptr != end || port < 1 || port > 65535) {
        throw std::invalid_argument("Port must be an integer from 1 to 65535");
    }
    return port;
}

void printUsage(const char* executable) {
    std::cout << "Usage: " << executable
              << " [--host 127.0.0.1] [--port 8080] [--db data/avl-studio.sqlite3]\n";
}

} // namespace

int main(int argc, char** argv) {
    try {
        const char* environmentPort = std::getenv("AVL_STUDIO_PORT");
        const char* environmentHost = std::getenv("AVL_STUDIO_HOST");
        const char* environmentDatabase = std::getenv("AVL_STUDIO_DB_PATH");
        std::string host = environmentHost == nullptr ? "127.0.0.1" : environmentHost;
        std::string databasePath = environmentDatabase == nullptr
            ? "data/avl-studio.sqlite3"
            : environmentDatabase;
        int port = environmentPort == nullptr ? 8080 : parsePort(environmentPort);

        for (int index = 1; index < argc; ++index) {
            const std::string argument = argv[index];
            if (argument == "--help") {
                printUsage(argv[0]);
                return 0;
            }
            if (argument == "--host" || argument == "--port" || argument == "--db") {
                if (index + 1 >= argc) {
                    throw std::invalid_argument("Missing value after " + argument);
                }
                const std::string value = argv[++index];
                if (argument == "--host") {
                    host = value;
                } else if (argument == "--port") {
                    port = parsePort(value);
                } else {
                    databasePath = value;
                }
                continue;
            }
            throw std::invalid_argument("Unknown option: " + argument);
        }

        ReservationStore reservations(databasePath);
        WebApplication application(reservations, AVL_STUDIO_ASSET_DIR);
        httplib::Server server;
        server.set_payload_max_length(16 * 1024);
        application.registerRoutes(server);

        std::cout << "AVL Studio is listening at http://" << host << ':' << port << "/\n"
                  << "Reservation database: " << databasePath << '\n'
                  << "Press Ctrl+C to stop the server.\n";
        if (!server.listen(host.c_str(), port)) {
            std::cerr << "Unable to listen on " << host << ':' << port
                      << ". Check that the host and port are available.\n";
            return 1;
        }
    } catch (const std::exception& error) {
        std::cerr << "AVL Studio startup failed: " << error.what() << '\n';
        printUsage(argv[0]);
        return 1;
    }
    return 0;
}
