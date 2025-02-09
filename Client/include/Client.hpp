/*
** EPITECH PROJECT, 2024
** R-Type [WSL: Ubuntu]
** File description:
** Client
*/

#pragma once

#include "Packet.hpp"

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/array.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <array>
#include <thread>
#include <mutex>
#include <csignal>
#include <unordered_map>
#include <boost/asio/steady_timer.hpp>
#include <queue>
#include <vector>
#include <map>

#define MAX_LENGTH 4096
#define BASE_AUDIO 50

namespace RType {
    enum class SpriteType {
        Enemy,
        Boss,
        Player,
        Bullet,
        Background,
        Start_button,
        EnemyBullet,
        Ball
    };

    class PacketElement
    {
        public:
            int action;
            int server_id;
            float new_x;
            float new_y;
    };

    class Frame {
    public:
        int frameId;
        std::vector<PacketElement> entityPackets;
    };

    class SpriteElement {
    public:
        sf::Sprite sprite;
        int id = -1;
    };

    class Client {
    public:
        Client(boost::asio::io_context& io_context, const std::string& host, short server_port, short client_port);
        ~Client();
        void send(const std::string& message);
        void start_receive();
        int main_loop();
        std::string createPacket(Network::PacketType type);
        void adjustVolume(float change);
        void handleKeyPress(sf::Keyboard::Key key, sf::RenderWindow& window);
        void sendExitPacket() { send(createPacket(Network::PacketType::DISCONNECTED)); }

    private:
        void handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred);
        void handle_send(const boost::system::error_code& error, std::size_t bytes_transferred);
        void run_receive();
        void createSprite(Frame &frame);
        void loadTextures();
        void drawSprites(sf::RenderWindow& window);
        void updateSpritePosition(Frame &frame);
        void UpdateGameStateLayers();
        void parseMessage(std::string packet_data);
        void parseFramePacket(const std::string& packet_data);
        void parseGameStatePacket(const std::string& packet_data);
        void destroySprite(Frame &frame);
        void checkWinCondition(Frame& frame);
        void processEvents(sf::RenderWindow& window);
        void initLobbySprites(sf::RenderWindow& window);
        void LoadSound();
        void LoadFont();
        void updatePacketLoss();
        std::string createMousePacket(Network::PacketType type, int x = 0, int y = 0);
        void start_send_timer();
        void handle_send_timer(const boost::system::error_code& error);

        sf::RenderWindow window;
        boost::asio::ip::udp::socket socket_;
        boost::asio::ip::udp::endpoint server_endpoint_;
        std::array<char, MAX_LENGTH> recv_buffer_;
        std::string received_data;
        std::mutex mutex_last_received_frame_id;
        std::mutex mutex_frameMap;
        std::thread receive_thread_;
        boost::asio::io_context& io_context_;
        std::vector<SpriteElement> sprites_;
        std::unordered_map<SpriteType, sf::Texture> textures_;
        std::vector<PacketElement> packets;
        std::map<int, Frame> frameMap;
        PacketElement gameStatePacket;
        sf::Clock frameClock;
        sf::Clock packetLossClock;
        int packetLossCount = 0;
        int currentFrameIndex = -1;
        int last_received_frame_id = -1;
        bool winGame = false;
        const sf::Time frameDuration = sf::milliseconds(10);
        const sf::Time packetLossDuration = sf::seconds(10);
        sf::SoundBuffer buffer_background_;
        sf::Sound sound_background_;
        sf::SoundBuffer buffer_shoot_;
        sf::Sound sound_shoot_;
        sf::Font font;
        sf::Text latencyText;
        sf::Text packetLossText;
        sf::Text winText;
        boost::asio::steady_timer send_timer_;
        std::queue<std::string> send_queue_;
    };
}
