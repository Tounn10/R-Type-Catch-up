/*
** EPITECH PROJECT, 2025
** R-Type [WSL: Ubuntu]
** File description:
** Pong Game
*/


#include "Server.hpp"
#include "Pong.hpp"
#include "AGame.hpp"
#include "Velocity.hpp"
#include "CollisionSystem.hpp"
#include <algorithm>
#include <iostream>
#include <random>
#include <thread>

Pong::Pong(RType::Server* server) : m_server(server) {
    std::srand(static_cast<unsigned int>(std::time(0)));
    registerComponents();
}

Pong::~Pong()
{
    playerActions.clear();
    entities.clear();
}

std::map<int, GeneralEntity>& Pong::getEntities() {
    return entities;
}

std::map<int, EngineFrame>& Pong::getEngineFrames() {
    return engineFrames;
}

void Pong::registerComponents()
{
    registry.register_component<Position>();
    registry.register_component<Velocity>();
    registry.register_component<Drawable>();
    registry.register_component<Controllable>();
    registry.register_component<Collidable>();
    registry.register_component<Projectile>();
}

void Pong::addPlayerAction(int playerId, int actionId) {
    std::lock_guard<std::mutex> lock(playerActionsMutex);
    playerActions.emplace_back(playerId, actionId);
}

void Pong::processPlayerActions(EngineFrame &frame) {
    for (auto& action : playerActions) {
        int playerId = action.getId();
        int actionId = action.getActionId();

        if (actionId > 0 && actionId < 5) {
            handlePlayerMove(playerId, actionId);
            action.setProcessed(true);
        }
    }
    std::lock_guard<std::mutex> lock(playerActionsMutex);
    deletePlayerAction();
}

void Pong::deletePlayerAction() {
    playerActions.erase(
        std::remove_if(playerActions.begin(), playerActions.end(),
            [](const PlayerAction& action) { return action.getProcessed(); }),
        playerActions.end());
}

const std::vector<PlayerAction>& Pong::getPlayerActions() const {
    return playerActions;
}

std::pair<float, float> Pong::getEntityPosition(int entityId) const
{
    auto it = entities.find(entityId);
    if (it == entities.end()) {
        throw std::out_of_range("Invalid entity ID");
    }

    const auto& positionComponent = it->second.getRegistry().get_components<Position>()[it->second.getEntity()];
    return {positionComponent->x, positionComponent->y};
}

void Pong::spawnEntity(GeneralEntity::EntityType type, float x, float y, EngineFrame &frame) {
    int entityId = id_to_set;
    if (type == GeneralEntity::EntityType::Bullet) {
        x += 50.0f;
        y += 25.0f;
    }

    entities.emplace(entityId, GeneralEntity(registry, type, x, y));

    std::string data = std::to_string(entityId) + ";" + std::to_string(x) + ";" + std::to_string(y) + "/";

    Network::PacketType packetType;
    switch (type) {
    case GeneralEntity::EntityType::Player:
        packetType = Network::PacketType::CREATE_PLAYER;
        break;
    case GeneralEntity::EntityType::Ball:
        packetType = Network::PacketType::CREATE_BALL;
        break;
    default:
        std::cerr << "Error: Unsupported entity type for spawning." << std::endl;
        return;
    }

    frame.frameInfos += m_server->createPacket(packetType, data);
    frame.frameInfos += m_server->createPacket(Network::PacketType::IMPORTANT_PACKET, "-1;-1;-1/");
    id_to_set++;
}

void Pong::killEntity(int entityId, EngineFrame &frame)
{
    auto it = entities.find(entityId);
    if (it != entities.end()) {
        it->second.getRegistry().kill_entity(it->second.getEntity());
        entities.erase(it);
        std::string data = std::to_string(entityId) + ";-1;-1/";
        frame.frameInfos += m_server->createPacket(Network::PacketType::DELETE, data);
        frame.frameInfos += m_server->createPacket(Network::PacketType::IMPORTANT_PACKET, "-1;-1;-1/");
    }
}

void Pong::moveBall(EngineFrame &frame) {
    const float maxY = 720.0f;
    const float minY = 0.0f;
    const float ballSpeedX = (lastPlayerHit == 1) ? 2.0f : -2.0f;
    float deltaY = ((std::rand() % 2001) / 1000.0f) - 1.0f;

    std::map<int, GeneralEntity> temp_entities = entities;
    for (auto& [id, entity] : temp_entities) {
        if (entity.getType() == GeneralEntity::EntityType::Ball) {
            auto [x, y] = getEntityPosition(id);
            float newX = x + ballSpeedX;
            float newY = y + deltaY;

            if (newY < minY) {
                deltaY = minY;
            } else if (newY > maxY) {
                deltaY = maxY;
            }

            if (newX < -50 || newX > 1330)
                gameOver = true;

            entities.find(id)->second.move(ballSpeedX, deltaY);
        }
    }
}

void Pong::checkCollisions(GeneralEntity::EntityType typeA, GeneralEntity::EntityType typeB, float thresholdX, float thresholdY, EngineFrame &frame) {
    auto tempEntities = entities; // Copy to avoid iterator invalidation

    for (auto& [idA, entityA] : tempEntities) {
        if (entityA.getType() != typeA) continue;
        auto [posXA, posYA] = getEntityPosition(idA);

        for (auto& [idB, entityB] : tempEntities) {
            if (entityB.getType() != typeB || idA == idB) continue;

            auto [posXB, posYB] = getEntityPosition(idB);

            if (std::abs(posXA - posXB) < thresholdX && std::abs(posYA - posYB) < thresholdY) {
                if (entities.find(idA) != entities.end() && entityA.getType() == GeneralEntity::EntityType::Player) {
                    lastPlayerHit = (idA == 0) ? 1 : 2;
                }
            }
        }
    }
}

void Pong::initializeplayers(int numPlayers, EngineFrame &frame) {
    for (int i = playerSpawned; i < numPlayers && playerSpawned < maxPlayers; ++i) {
        frame.frameInfos += m_server->createPacket(Network::PacketType::CREATE_BACKGROUND, "-100;0;0/");
        if (playerSpawned == 0) {
            spawnEntity(GeneralEntity::EntityType::Player, 100.0f, 360.0f, frame);
            playerSpawned++;
        } else if (playerSpawned == 1) {
            spawnEntity(GeneralEntity::EntityType::Player, 1100.0f, 360.0f, frame);
            playerSpawned++;
        }
        frame.frameInfos += m_server->createPacket(Network::PacketType::IMPORTANT_PACKET, "-1;-1;-1/");
    }
}

void Pong::CheckWinCondition(EngineFrame &frame) {
    if (gameOver) {
        frame.frameInfos += m_server->createPacket(Network::PacketType::WIN, "-1;-1;-1/");
    }
}

void Pong::update(EngineFrame &frame) {
    registry.run_systems();
    initializeplayers(m_server->getClients().size(), frame);
    processPlayerActions(frame);

    if (currentBalls < maxBalls && playerSpawned == 2) {
        spawnBallRandomly(frame);
    }

    checkCollisions(GeneralEntity::EntityType::Player, GeneralEntity::EntityType::Ball, 50.0f, 70.0f, frame);
    moveBall(frame);
    CheckWinCondition(frame);
}

void Pong::run(int numPlayers) {
    int frameId = 0;
    int last_frame_sent = 0;

    while (true) {
        if (frameClock.getElapsedTime() >= frameDuration) {
            frameClock.restart();
            EngineFrame new_frame;
            m_server->server_mutex.lock();
            update(new_frame);
            engineFrames.emplace(frameId++, new_frame);
            m_server->server_mutex.unlock();
        }
    }
}

void Pong::handlePlayerMove(int playerId, int actionId) {
    float moveDistance = 3.0f;
    float x = 0.0f;
    float y = 0.0f;

    if (actionId == 3) { // Up
        y = -moveDistance;
    } else if (actionId == 4) { // Down
        y = moveDistance;
    }
    auto it = entities.find(playerId);
    if (it != entities.end()) {
        it->second.move(x, y);
    } else {
        std::cerr << "[ERROR] Player ID " << playerId << " not found." << std::endl;
    }
}

float Pong::randomFloat(float min, float max) {
    return min + static_cast<float>(std::rand()) / (RAND_MAX / (max - min));
}

void Pong::spawnBallRandomly(EngineFrame &frame) {
    for (int i = 0; i < maxBalls; ++i) {

        float x = 600;
        float y = 400;

        spawnEntity(GeneralEntity::EntityType::Ball, x, y, frame);
    }
    currentBalls++;
}

size_t Pong::getEntityCount() const {
    return entities.size();
}

extern "C" AGame* create_game(void* server) {
    return new Pong(static_cast<RType::Server*>(server));
}
