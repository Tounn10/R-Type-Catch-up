/*
** EPITECH PROJECT, 2024
** R-Type [WSL: Ubuntu]
** File description:
** main
*/

#include "Server.hpp"
#include "Errors/Throws.hpp"
#include "Packet.hpp"
#include "ThreadSafeQueue.hpp"
#include "PacketHandler.hpp"
#include "GameState.hpp"
#include <dlfcn.h>

short parsePort(int ac, char **av)
{
    if (ac != 3) {
        std::cerr << "Usage: " << av[0] << " <port>" << std::endl;
        throw RType::InvalidPortException("Invalid number of arguments");
    }
    try {
        return std::stoi(av[1]);
    } catch (const std::exception& e) {
        throw RType::InvalidPortException("Invalid port number");
    }
}

void runServer(short port, const std::string& gameName) {
    try {
        boost::asio::io_context io_context;
        ThreadSafeQueue<Network::Packet> packetQueue;

        RType::Server server(io_context, port, packetQueue, nullptr);

        // Load the correct game library based on the game name
        std::string libPath = "./R-Type/lib" + gameName + ".so";
        void* handle = dlopen(libPath.c_str(), RTLD_LAZY);
        if (!handle) {
            std::cerr << "Error loading library: " << dlerror() << std::endl;
            return;
        }

        // Load the factory function
        typedef AGame* (*CreateGameFunc)(void*);
        CreateGameFunc create_game = (CreateGameFunc)dlsym(handle, "create_game");

        if (!create_game) {
            std::cerr << "Error loading function: " << dlerror() << std::endl;
            dlclose(handle);
            return;
        }

        // Create an instance of the game dynamically
        AGame* game = create_game(&server);
        server.setGameState(game);  // Set the game dynamically

        // Start handling packets
        Network::PacketHandler packetHandler(packetQueue, *game, server);
        packetHandler.start();

        std::cout << "Server started\nListening on UDP port " << port << " running " << gameName << std::endl;

        std::thread io_thread([&io_context] { io_context.run(); });
        std::thread serverThread([&server] { server.run(); });

        if (io_thread.joinable()) io_thread.join();
        if (serverThread.joinable()) serverThread.join();

        packetHandler.stop();

        // Cleanup
        delete game;
        dlclose(handle);

    } catch (const boost::system::system_error& e) {
        if (e.code() == boost::asio::error::access_denied) {
            throw RType::PermissionDeniedException("Permission denied");
        } else {
            throw;
        }
    }
}

int main(int ac, char **av)
{
    try {
        short port = parsePort(ac, av);
        std::string gameName = av[2];
        runServer(port, gameName);
    } catch (const RType::NtsException& e) {
        std::cerr << "Exception: " << e.what() << " (Type: " << e.getType() << ")" << std::endl;
    } catch (const std::exception& e) {
        throw RType::StandardException(e.what());
    } catch (...) {
        throw RType::UnknownException("Unknown exception occurred");
    }
    return 0;
}