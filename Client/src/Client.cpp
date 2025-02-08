/*
** EPITECH PROJECT, 2024
** R-Type [WSL: Ubuntu]
** File description:
** Client
*/

#include "Client.hpp"

#include <string>
#include <X11/Xlibint.h>

using boost::asio::ip::udp;

RType::Client::Client(boost::asio::io_context& io_context, const std::string& host, short server_port, short client_port)
    : socket_(io_context, udp::endpoint(udp::v4(), client_port)), io_context_(io_context), window(sf::VideoMode(1280, 720), "R-Type Client"), send_timer_(io_context) // Initialize send_timer_
{
    udp::resolver resolver(io_context);
    udp::resolver::query query(udp::v4(), host, std::to_string(server_port));
    server_endpoint_ = *resolver.resolve(query).begin();
    std::cout << "Connected to " << host << ":" << server_port << " from client port " << client_port << std::endl;

    start_receive();
    start_send_timer(); // Start the send timer
    receive_thread_ = std::thread(&Client::run_receive, this);
}

RType::Client::~Client()
{
    io_context_.stop();
    socket_.close();
    if (receive_thread_.joinable()) {
        receive_thread_.join();
    }
    frameMap.clear();
    sprites_.clear();
}

void RType::Client::send(const std::string& message)
{
    socket_.async_send_to(
        boost::asio::buffer(message), server_endpoint_,
        boost::bind(&Client::handle_send, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
}

void RType::Client::start_receive()
{
    socket_.async_receive_from(
        boost::asio::buffer(recv_buffer_), server_endpoint_,
        boost::bind(&RType::Client::handle_receive, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
}

void RType::Client::handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    if (!error || error == boost::asio::error::message_size) {
        received_data.assign(recv_buffer_.data(), bytes_transferred);
        parseMessage(received_data);
        start_receive();
    } else {
        std::cerr << "[DEBUG] Error receiving: " << error.message() << std::endl;
        start_receive();
    }
}

void RType::Client::handle_send(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    if (!error) {
        // std::cout << "[DEBUG] Message sent." << std::endl;
    } else {
        std::cerr << "[DEBUG] Error sending: " << error.message() << std::endl;
    }
}

void RType::Client::run_receive()
{
    io_context_.run();
}

void RType::Client::createSprite(Frame& frame) {
    static const std::unordered_map<int, SpriteType> actionToSpriteType = {
        {22, SpriteType::Enemy},
        {23, SpriteType::Boss},
        {24, SpriteType::Player},
        {25, SpriteType::Bullet},
        {26, SpriteType::Background},
        {27, SpriteType::EnemyBullet}
    };

    for (auto& packet : frame.entityPackets) {
        auto it = actionToSpriteType.find(packet.action);
        if (it != actionToSpriteType.end()) {
            auto spriteIt = std::find_if(sprites_.begin(), sprites_.end(), [&](const SpriteElement& sprite) {
                return sprite.id == packet.server_id;
            });
            if (spriteIt == sprites_.end()) {
                SpriteElement spriteElement;
                spriteElement.sprite.setTexture(textures_[it->second]);
                spriteElement.sprite.setPosition(packet.new_x, packet.new_y);
                spriteElement.id = packet.server_id;
                sprites_.push_back(spriteElement);
            } else {
                std::cout << "[DEBUG] Sprite with Server ID: " << packet.server_id << " already exists." << std::endl;
            }
        }
    }
}


void RType::Client::destroySprite(Frame& frame) {
    for (auto& packet : frame.entityPackets) {
        if (packet.action == 28) {
            auto it = std::remove_if(sprites_.begin(), sprites_.end(), [&](const SpriteElement& sprite) {
                return sprite.id == packet.server_id;
            });
            sprites_.erase(it, sprites_.end());
        }
    }
}



void RType::Client::updateSpritePosition(Frame& frame) {
    for (auto& packet : frame.entityPackets) {
        if (packet.action == 29) {
            auto it = std::find_if(sprites_.begin(), sprites_.end(), [&](const SpriteElement& sprite) {
                return sprite.id == packet.server_id;
            });
            if (it != sprites_.end()) {
                it->sprite.setPosition(packet.new_x, packet.new_y);
            }
        }
    }
}

void RType::Client::UpdateGameStateLayers() {
    if (gameStatePacket.action == 31) {
        initLobbySprites(this->window);
    } else if (gameStatePacket.action == 3) {
        for (auto it = sprites_.begin(); it != sprites_.end(); ++it) {
            if (it->id == -101) {
                sprites_.erase(it);
                break;
            }
        }
    }
    gameStatePacket.action = -1;
}

void RType::Client::checkWinCondition(Frame& frame) {
    for (auto& packet : frame.entityPackets) {
        if (packet.action == 35) {
            winGame = true;
        }
    }
}

void RType::Client::loadTextures() //make sure to have the right textures in the right folder
{
    textures_[RType::SpriteType::Enemy].loadFromFile("../assets/enemy.png");
    textures_[RType::SpriteType::Boss].loadFromFile("../assets/boss.png");
    textures_[RType::SpriteType::Player].loadFromFile("../assets/player.png");
    textures_[RType::SpriteType::Bullet].loadFromFile("../assets/bullet.png");
    textures_[RType::SpriteType::Background].loadFromFile("../assets/background.png");
    textures_[RType::SpriteType::Start_button].loadFromFile("../assets/start_button.png");
    textures_[RType::SpriteType::EnemyBullet].loadFromFile("../assets/enemy_bullet.png");
}

void RType::Client::drawSprites(sf::RenderWindow& window)
{
    for (auto& spriteElement : sprites_) {
        window.draw(spriteElement.sprite);
    }
    window.draw(latencyText);
    window.draw(packetLossText);
    if (winGame)
        window.draw(winText);
}

void RType::Client::parseMessage(std::string packet_data)
{
    if (packet_data.empty()) {
        std::cerr << "[ERROR] Empty packet data." << std::endl;
        return;
    }

    if (packet_data.find(':') != std::string::npos) {
        parseFramePacket(packet_data);
    } else {
        parseGameStatePacket(packet_data);
    }
}

void RType::Client::parseFramePacket(const std::string& packet_data)
{
    std::stringstream ss(packet_data);
    std::string frame_segment;

    std::getline(ss, frame_segment, ':');
    int frame_id;
    try {
        frame_id = std::stoi(frame_segment);
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Invalid Frame ID format: " << frame_segment << std::endl;
        return;
    }

    Frame new_frame;
    new_frame.frameId = frame_id;

    if (ss.eof()) {
        mutex_frameMap.lock();
        frameMap.emplace(frame_id, new_frame);
        mutex_frameMap.unlock();
        return;
    }

    while (std::getline(ss, frame_segment, '/')) {
        std::vector<std::string> elements;
        std::stringstream packet_ss(frame_segment);
        std::string segment;

        while (std::getline(packet_ss, segment, ';')) {
            elements.push_back(segment);
        }

        if (elements.size() != 4) {
            std::cerr << "[ERROR] Invalid subpacket format: " << frame_segment << std::endl;
            continue;
        }

        try {
            PacketElement packetElement;
            uint8_t packet_type = static_cast<uint8_t>(frame_segment[0]);
            packetElement.action = static_cast<int>(packet_type);
            packetElement.server_id = std::stoi(elements[1]);
            packetElement.new_x = std::stof(elements[2]);
            packetElement.new_y = std::stof(elements[3]);

            if (packetElement.action == 33) {
                send_queue_.push(createPacket(Network::PacketType::IMPORTANT_PACKET_RECEIVED) + ";" + std::to_string(frame_id));
            } else
                new_frame.entityPackets.push_back(packetElement);

        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Failed to parse subpacket: " << e.what() << std::endl;
        }
    }

    packetLossCount = frame_id - (last_received_frame_id + 1);
    mutex_last_received_frame_id.lock();
    last_received_frame_id = frame_id;
    mutex_last_received_frame_id.unlock();
    mutex_frameMap.lock();
    frameMap.emplace(new_frame.frameId, new_frame);
    mutex_frameMap.unlock();
}

void RType::Client::parseGameStatePacket(const std::string& packet_data)
{
    std::stringstream ss(packet_data);
    std::vector<std::string> elements;
    std::string segment;

    while (std::getline(ss, segment, ';')) {
        elements.push_back(segment);
    }

    if (elements.size() != 4) {
        std::cerr << "[ERROR] Invalid game state packet format: " << packet_data << std::endl;
        return;
    }

    try {
        PacketElement packetElement;
        uint8_t packet_type = static_cast<uint8_t>(packet_data[0]);
        packetElement.action = static_cast<int>(packet_type);
        packetElement.server_id = std::stoi(elements[1]);
        packetElement.new_x = std::stof(elements[2]);
        packetElement.new_y = std::stof(elements[3]);

        if (packetElement.action == 31 || packetElement.action == 3 || packetElement.action == 30) {
            gameStatePacket = packetElement;
        }
        if (packetElement.action == 32) {
            auto now = std::chrono::steady_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            long long sentTime = packetElement.server_id;
            long long latency = ms - sentTime;
            latencyText.setString("Latency: " + std::to_string(latency) + " ms");
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Failed to parse game state packet: " << e.what() << std::endl;
    }
}

void RType::Client::LoadSound()
{
    if (!buffer_background_.loadFromFile("../assets/sound.wav")) {
        std::cerr << "[ERROR] loading sound file" << std::endl;
    }
    if (!buffer_shoot_.loadFromFile("../assets/shoot.wav")) {
        std::cerr << "[ERROR] loading sound file" << std::endl;
    }
    sound_background_.setBuffer(buffer_background_);
    sound_shoot_.setBuffer(buffer_shoot_);
    sound_shoot_.setVolume(BASE_AUDIO);
    sound_background_.setVolume(BASE_AUDIO);
    sound_background_.setLoop(true);
    sound_background_.play();
}

void RType::Client::LoadFont()
{
    if (!font.loadFromFile("../assets/test.ttf")) {
        std::cerr << "Error loading font\n";
    }

    packetLossText.setFont(font);
    packetLossText.setCharacterSize(24);
    packetLossText.setFillColor(sf::Color::White);
    packetLossText.setPosition(10, 40);
    latencyText.setFont(font);
    latencyText.setCharacterSize(24);
    latencyText.setFillColor(sf::Color::White);
    latencyText.setPosition(10, 10);
    winText.setFont(font);
    winText.setCharacterSize(50);
    winText.setFillColor(sf::Color::White);
    winText.setPosition(600, 300);
    winText.setString("You Win !");
}

void RType::Client::updatePacketLoss() {

    if (packetLossClock.getElapsedTime().asSeconds() >= packetLossDuration.asSeconds()) {
        packetLossCount = 0;
        packetLossClock.restart();
    }

    packetLossText.setString("Packet Loss: " + std::to_string(packetLossCount));
}


int RType::Client::main_loop()
{
    loadTextures();
    send(createPacket(Network::PacketType::REQCONNECT));
    LoadSound();
    LoadFont();

    while (this->window.isOpen()) {
        if (last_received_frame_id != -1 && currentFrameIndex == -1) {
            mutex_frameMap.lock();
            currentFrameIndex = frameMap.begin()->first;
            mutex_frameMap.unlock();
        }
        processEvents(this->window);
        UpdateGameStateLayers();

        if (frameClock.getElapsedTime().asMilliseconds() >= frameDuration.asMilliseconds()) {
            if (frameClock.getElapsedTime().asMilliseconds() > frameDuration.asMilliseconds()) {
                std::cerr << "[WARNING] Frame took too long to process: " << frameClock.getElapsedTime().asMilliseconds() << "ms" << std::endl;
            }
            sf::Time elapsed = frameClock.getElapsedTime() - frameDuration; // Mettre compensation pour éviter le décalage client/serveur (implementer des 2 cotés)
            frameClock.restart();

            if (!frameMap.empty()) {
                mutex_frameMap.lock();
                auto it = frameMap.find(currentFrameIndex);
                updatePacketLoss();
                Frame currentFrame = it->second;
                mutex_frameMap.unlock();
                createSprite(currentFrame);
                destroySprite(currentFrame);
                updateSpritePosition(currentFrame);
                checkWinCondition(currentFrame);
                currentFrameIndex++;
            }
            this->window.clear();
            drawSprites(window);
            this->window.display();
        }
    }
    sendExitPacket();
    return 0;
}

std::string RType::Client::createPacket(Network::PacketType type)
{
    Network::Packet packet;
    packet.type = type;
    std::string packet_str;
    packet_str.push_back(static_cast<uint8_t>(type));
    return packet_str;
}

std::string RType::Client::createMousePacket(Network::PacketType type, int x, int y)
{
    Network::Packet packet;
    packet.type = type;

    std::ostringstream packet_str;
    packet_str << static_cast<uint8_t>(type) << ";" << x << ";" << y;

    return packet_str.str();
}

std::string deserializePacket(const std::string& packet_str)
{
    Network::Packet packet;
    packet.type = static_cast<Network::PacketType>(packet_str[0]);
    return packet_str;
}

void RType::Client::processEvents(sf::RenderWindow& window)
{
    sf::Event event;
    while (window.pollEvent(event)) {
        switch (event.type) {
            case sf::Event::Closed:
                window.close();
                break;
            case sf::Event::MouseButtonPressed: {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                if (!sprites_.empty()) {
                    if (sprites_.back().id == -101) {
                        send_queue_.push(createPacket(Network::PacketType::GAME_START));
                        sprites_.pop_back();
                    } else {
                        send_queue_.push(createMousePacket(Network::PacketType::PLAYER_SHOOT, mousePos.x, mousePos.y));
                        sound_shoot_.play();
                    }
                }
                break;
            }
            case sf::Event::KeyPressed:
                handleKeyPress(event.key.code, window);
                break;

            default:
                break;
        }
    }
}

void RType::Client::handleKeyPress(sf::Keyboard::Key key, sf::RenderWindow& window)
{
    switch (key) {
        case sf::Keyboard::Right:
            send_queue_.push(createPacket(Network::PacketType::PLAYER_RIGHT));
            break;

        case sf::Keyboard::Left:
            send_queue_.push(createPacket(Network::PacketType::PLAYER_LEFT));
            break;

        case sf::Keyboard::Up:
            send_queue_.push(createPacket(Network::PacketType::PLAYER_UP));
            break;

        case sf::Keyboard::Down:
            send_queue_.push(createPacket(Network::PacketType::PLAYER_DOWN));
            break;

        case sf::Keyboard::Q:
            sendExitPacket();
            window.close();
            break;

        case sf::Keyboard::M:
            std::cout << "[DEBUG] Sending M: " << std::endl;
            send_queue_.push(createPacket(Network::PacketType::OPEN_MENU));
            break;

        case sf::Keyboard::Space:
            std::cout << "[DEBUG] Sending Space: " << std::endl;
            send_queue_.push(createPacket(Network::PacketType::PLAYER_SHOOT));
            break;

        case sf::Keyboard::Escape:
            initLobbySprites(window);
            send_queue_.push(createPacket(Network::PacketType::GAME_END));
            break;

        case sf::Keyboard::Num1:
            adjustVolume(-5);
            break;

        case sf::Keyboard::Num2:
            adjustVolume(5);
            break;

        default:
            break;
    }
}

void RType::Client::adjustVolume(float change)
{
    float newVolume = sound_background_.getVolume() + change;
    if (newVolume < 0) newVolume = 0;
    if (newVolume > 100) newVolume = 100;
    sound_background_.setVolume(newVolume);
}

void RType::Client::initLobbySprites(sf::RenderWindow& window)
{
    sprites_.clear();

    SpriteElement backgroundElement;
    backgroundElement.sprite.setTexture(textures_[SpriteType::Background]);
    backgroundElement.sprite.setPosition(0, 0);
    backgroundElement.id = -100;

    SpriteElement buttonElement;
    buttonElement.sprite.setTexture(textures_[SpriteType::Start_button]);
    buttonElement.sprite.setPosition(window.getSize().x / 2 - textures_[SpriteType::Start_button].getSize().x / 2, window.getSize().y / 2 - textures_[SpriteType::Start_button].getSize().y / 2);
    buttonElement.id = -101;

    sprites_.push_back(backgroundElement);
    sprites_.push_back(buttonElement);
}

void RType::Client::start_send_timer() {
    send_timer_.expires_after(std::chrono::milliseconds(1));
    send_timer_.async_wait(boost::bind(&Client::handle_send_timer, this, boost::asio::placeholders::error));
}

void RType::Client::handle_send_timer(const boost::system::error_code& error) {
    if (!error) {
        if (!send_queue_.empty()) {
            std::string message = send_queue_.front();
            send_queue_.pop();
            send(message);
        }
        start_send_timer();
    } else {
        std::cerr << "[DEBUG] Timer error: " << error.message() << std::endl;
    }
}
