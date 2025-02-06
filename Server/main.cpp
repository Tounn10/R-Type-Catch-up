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
    if (ac != 2) {
        std::cerr << "Usage: " << av[0] << " <port>" << std::endl;
        throw RType::InvalidPortException("Invalid number of arguments");
    }
    try {
        return std::stoi(av[1]);
    } catch (const std::exception& e) {
        throw RType::InvalidPortException("Invalid port number");
    }
}

void runServer(short port) {
    try {
        boost::asio::io_context io_context;
        ThreadSafeQueue<Network::Packet> packetQueue;

        RType::Server server(io_context, port, packetQueue, nullptr);

        // Step 1: Open the shared library
        void* handle = dlopen("./R-Type/libGameState.so", RTLD_LAZY);
        if (!handle) {
            std::cerr << "Error loading library: " << dlerror() << std::endl;
            return;
        }

        // Step 2: Load the factory function
        typedef GameState* (*CreateGameFunc)(void*);
        CreateGameFunc create_game = (CreateGameFunc)dlsym(handle, "create_game");

        if (!create_game) {
            std::cerr << "Error loading function: " << dlerror() << std::endl;
            dlclose(handle);
            return;
        }

        // Step 3: Create an instance of GameState dynamically
        GameState* game = create_game(&server);
        server.setGameState(game);  // Correctly set the GameState object

        // Start handling packets with the correctly typed GameState
        Network::PacketHandler packetHandler(packetQueue, *game, server);
        packetHandler.start();

        std::cout << "Server started\nListening on UDP port " << port << std::endl;

        std::thread io_thread([&io_context] {
            io_context.run();
        });

        std::thread serverThread([&server] {
            server.run();
        });

        if (io_thread.joinable())
            io_thread.join();
        if (serverThread.joinable())
            serverThread.join();

        packetHandler.stop();

        // Step 4: Cleanup
        delete game;  // Delete dynamically allocated GameState
        dlclose(handle);  // Close the shared library

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
        runServer(port);
    } catch (const RType::NtsException& e) {
        std::cerr << "Exception: " << e.what() << " (Type: " << e.getType() << ")" << std::endl;
    } catch (const std::exception& e) {
        throw RType::StandardException(e.what());
    } catch (...) {
        throw RType::UnknownException("Unknown exception occurred");
    }
    return 0;
}