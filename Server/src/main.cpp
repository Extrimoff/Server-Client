#include "Network/Server/Server.hpp"
#include <print>

int main(int argc, const char* argv) {
    Server server(8081, { 1, 1, 1 });
    try {
        //Start server
        if (server.start() == ServerStatus::up) {
            std::println(stderr, 
                "Server listen on port: {}\n"
                "Server handling thread pool size: {}",
                server.getPort(), server.getThreadPool().getThreadCount());
            server.joinLoop();
            return EXIT_SUCCESS;
        }
        else {
            std::println(stderr, "Server start error! Error code: {}", static_cast<int>(server.getStatus()));
            return EXIT_FAILURE;
        }

    }
    catch (std::exception& except) {
        std::println(stderr, "{}", except.what());
        return EXIT_FAILURE;
    }
}