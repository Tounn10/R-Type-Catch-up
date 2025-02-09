/*
** EPITECH PROJECT, 2025
** R-Type [WSL: Ubuntu]
** File description:
** Server
*/

#pragma once

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <queue>
#include <map>
#include <boost/asio/steady_timer.hpp>

#include "ThreadSafeQueue.hpp"
#include "Packet.hpp"
#include "ClientRegister.hpp"
#include "GameState.hpp"

typedef std::map<uint32_t, ClientRegister> ClientList;

#define MAX_LENGTH 4096

using namespace boost::placeholders; // Used for Boost.Asio asynchronous operations to bind placeholders for callback functions

namespace RType {
    class Server {
    public:
        Server(boost::asio::io_context& io_context, short port, ThreadSafeQueue<Network::Packet>& packetQueue, GameState* game = nullptr);
        ~Server();

        void run();
        void handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred);
        void send_to_client(const std::string& message, const boost::asio::ip::udp::endpoint& client_endpoint);
        void setGameState(AGame* game);
        void Broadcast(const std::string& message);
        void SendFrame(EngineFrame &frame, int frameId);
        void PacketFactory(EngineFrame &frame);
        bool hasPositionChanged(int id, float x, float y, std::unordered_map<int, std::pair<float, float>>& lastKnownPositions);
        void sendAllEntitiesToNewClients(EngineFrame &frame);
        void resendImportPackets();
        void SendLatencyCheck();

        Network::ReqConnect reqConnectData(boost::asio::ip::udp::endpoint& client_endpoint);
        Network::DisconnectData disconnectData(boost::asio::ip::udp::endpoint& client_endpoint);
        Network::Packet deserializePacket(const std::string& packet_str);
        std::string createPacket(const Network::PacketType& type, const std::string& data);

        const ClientList& getClients() const { return clients_; }
        const udp::endpoint& getRemoteEndpoint() const { return remote_endpoint_; }

        ClientList clients_;
        uint32_t _nbClients;
        std::mutex clients_mutex_;
        std::mutex server_mutex;
        bool m_running;
        std::map<int, std::pair<EngineFrame, sf::Clock>> unacknowledgedPackets;

    private:
        using PacketHandler = std::function<void(const std::vector<std::string>&)>;
        void start_receive();
        uint32_t createClient(boost::asio::ip::udp::endpoint& client_endpoint);
        void start_send_timer();
        void handle_send_timer(const boost::system::error_code& error);

        udp::socket socket_;
        udp::endpoint remote_endpoint_;
        std::array<char, MAX_LENGTH> recv_buffer_;
        ThreadSafeQueue<Network::Packet>& m_packetQueue;
        std::unordered_map<std::string, std::function<void(const std::vector<std::string>&)>> packet_handlers_;
        std::unordered_map<Network::PacketType, void(*)(const Network::Packet&)> m_handlers;
        AGame* m_game;
        std::queue<std::string> send_queue_;
        boost::asio::steady_timer send_timer_;
        std::queue<uint32_t> available_ids_;
        sf::Clock latencyClock;
        const sf::Time LatencyRefreshDuration = sf::milliseconds(200);
    };
}

