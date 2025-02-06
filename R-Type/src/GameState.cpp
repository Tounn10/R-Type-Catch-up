/*
** EPITECH PROJECT, 2025
** R-Type [WSL: Ubuntu]
** File description:
** GameState
*/

#include "Server.hpp"
#include "GameState.hpp"
#include "AGame.hpp"
#include "Velocity.hpp"
#include "CollisionSystem.hpp"
#include <algorithm>
#include <iostream>
#include <thread>

GameState::GameState(RType::Server* server) : m_server(server), rng(std::random_device()()), distX(0.0f, 800.0f), distY(0.0f, 600.0f),
      distTime(1000, 5000), enemiesPerWave(5), nextEnemyId(0), nextBossId(0), currentWave(0) {
    registerComponents();
}

GameState::~GameState()
{
    playerActions.clear();
    entities.clear();
}

std::map<int, GeneralEntity>& GameState::getEntities() {
    return entities;
}

std::map<int, EngineFrame>& GameState::getEngineFrames() {
    return engineFrames;
}

void GameState::registerComponents()
{
    registry.register_component<Position>();
    registry.register_component<Velocity>();
    registry.register_component<Drawable>();
    registry.register_component<Controllable>();
    registry.register_component<Collidable>();
    registry.register_component<Projectile>();
}

void GameState::addPlayerAction(int playerId, int actionId) {
    std::cout << "Player " << playerId << " performed action " << actionId << std::endl;
    std::lock_guard<std::mutex> lock(playerActionsMutex);
    playerActions.emplace_back(playerId, actionId);
}

void GameState::processPlayerActions(EngineFrame &frame) {
    for (auto& action : playerActions) {
        int playerId = action.getId();
        int actionId = action.getActionId();

        if (actionId > 0 && actionId < 5) { // Change by real action ID defined in server
            handlePlayerMove(playerId, actionId);
            action.setProcessed(true);
        } else if (actionId == 5) {
            // Change by real action ID defined in server
            std::map<int, GeneralEntity> temp_entities = entities;
            for (const auto& [id, entity] : temp_entities) {
                if (entity.getType() == GeneralEntity::EntityType::Player && id == playerId) {
                    auto[x, y] = getEntityPosition(id);
                    spawnEntity(GeneralEntity::EntityType::Bullet, x, y, frame);
                    break;
                }
            }
            action.setProcessed(true);
        }
    }
    std::lock_guard<std::mutex> lock(playerActionsMutex);
    deletePlayerAction();
}

void GameState::deletePlayerAction() { //deletes all process elements from the vector of player actions and resizes vector
    playerActions.erase(
        std::remove_if(playerActions.begin(), playerActions.end(),
            [](const PlayerAction& action) { return action.getProcessed(); }),
        playerActions.end());
}

const std::vector<PlayerAction>& GameState::getPlayerActions() const {
    return playerActions;
}

std::pair<float, float> GameState::getEntityPosition(int entityId) const
{
    auto it = entities.find(entityId);
    if (it == entities.end()) {
        throw std::out_of_range("Invalid entity ID");
    }

    const auto& positionComponent = it->second.getRegistry().get_components<Position>()[it->second.getEntity()];
    return {positionComponent->x, positionComponent->y};
}

void GameState::spawnEntity(GeneralEntity::EntityType type, float x, float y, EngineFrame &frame) {
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
    case GeneralEntity::EntityType::Enemy:
        packetType = Network::PacketType::CREATE_ENEMY;
        break;
    case GeneralEntity::EntityType::Boss:
        packetType = Network::PacketType::CREATE_BOSS;
        break;
    case GeneralEntity::EntityType::Bullet:
        packetType = Network::PacketType::CREATE_BULLET;
        break;
    default:
        std::cerr << "Error: Unsupported entity type for spawning." << std::endl; //Becareful to not increment id_to_set if the entity type is not supported
        return;
    }

    frame.frameInfos += m_server->createPacket(packetType, data);
    frame.frameInfos += m_server->createPacket(Network::PacketType::IMPORTANT_PACKET, "-1;-1;-1/");
    id_to_set++;
}

void GameState::killEntity(int entityId, EngineFrame &frame)
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

void GameState::moveBullets(EngineFrame &frame) {
    const float maxX = 800.0f;

    std::map <int, GeneralEntity> temp_entities = entities;
    for (auto& [id, entity] : temp_entities) {
        if (entity.getType() == GeneralEntity::EntityType::Bullet) {
            auto [x, y] = getEntityPosition(id);
            float newX = x + 1.0f;
            if (newX > maxX) {
                killEntity(id, frame);
            } else {
                entities.find(id)->second.move(1.0f, 0.0f);
            }
        }
    }
}

void GameState::checkCollisions(GeneralEntity::EntityType typeA, GeneralEntity::EntityType typeB, float collisionThreshold, EngineFrame &frame) {
    auto tempEntities = entities; // Copy to avoid iterator invalidation

    for (const auto& [idA, entityA] : tempEntities) {
        if (entityA.getType() != typeA) continue;
        auto [posXA, posYA] = getEntityPosition(idA);

        for (const auto& [idB, entityB] : tempEntities) {
            if (entityB.getType() != typeB || idA == idB) continue;

            auto [posXB, posYB] = getEntityPosition(idB);
            float distance = std::sqrt(std::pow(posXB - posXA, 2) + std::pow(posXB - posXA, 2));

            if (distance < collisionThreshold) {
                killEntity(idA, frame);
                killEntity(idB, frame);
            }
        }
    }
}

void GameState::moveEnemies(EngineFrame &frame) {
    static int direction = 0;
    const float moveDistance = 1.0f;

    std::map<int, GeneralEntity> temp_entities = entities;
    for (auto& [id, entity] : temp_entities) {
        if (entity.getType() == GeneralEntity::EntityType::Enemy) {
            float x = 0.0f, y = 0.0f;

            switch (direction) {
                case 0: y = -moveDistance; break; // Up
                case 1: x = -moveDistance; break; // Left
                case 2: y = moveDistance; break;  // Down
                case 3: x = moveDistance; break;  // Right
            }

            entities.find(id)->second.move(x, y);

            if (rand() % 100 < 0.1) {
                auto[x, y] = getEntityPosition(id);
                spawnEntity(GeneralEntity::EntityType::Bullet, x + 50.0f, y + 25.0f, frame);
            }
        }
    }

    direction = (direction + 1) % 4;
}

//Old GameState functions

void GameState::initializeplayers(int numPlayers, EngineFrame &frame) {
    for (int i = countPlayers(); i < numPlayers; ++i) {
        spawnEntity(GeneralEntity::EntityType::Player, 100.0f * (i + 1.0f), 100.0f, frame);
    }
}

void GameState::update(EngineFrame &frame) {
    registry.run_systems();
    initializeplayers(m_server->getClients().size(), frame);
    processPlayerActions(frame);

    //if (areEnemiesCleared()) {
    //    spawnEnemiesRandomly(new_frame);
    //}
    //moveBullets(frame);
    //checkBulletEnemyCollisions(frame);
    //moveEnemies(frame);
}

void GameState::run(int numPlayers) {
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


void GameState::handlePlayerMove(int playerId, int actionId) {
    float moveDistance = 1.0f;
    float x = 0.0f;
    float y = 0.0f;

    if (actionId == 1) { // Left
        x = -moveDistance;
    } else if (actionId == 2) { // Right
        x = moveDistance;
    } else if (actionId == 3) { // Up
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

int GameState::countPlayers() const {
    return std::count_if(entities.begin(), entities.end(), [](const auto& pair) {
        return pair.second.getType() == GeneralEntity::EntityType::Player;
    });
}

bool GameState::isBossSpawned() const {
    return std::any_of(entities.begin(), entities.end(), [](const auto& pair) {
        return pair.second.getType() == GeneralEntity::EntityType::Boss;
    });
}

bool GameState::areEnemiesCleared() const {
    return std::none_of(entities.begin(), entities.end(), [](const auto& pair) {
        return pair.second.getType() == GeneralEntity::EntityType::Enemy;
    });
}

void GameState::startNextWave(EngineFrame &frame) {
    currentWave++;
    enemiesPerWave += 5; // Increase the number of enemies per wave
    for (int i = 0; i < enemiesPerWave; ++i) {
        spawnEntity(GeneralEntity::EntityType::Enemy, distX(rng), distY(rng), frame);
    }
}

void GameState::spawnEnemiesRandomly(EngineFrame &frame) {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSpawnTime).count();

    // Check time and max limit for enemy spawning
    if (elapsed > distTime(rng) &&
        std::count_if(entities.begin(), entities.end(), [](const auto& pair) {
            return pair.second.getType() == GeneralEntity::EntityType::Enemy;
        }) < 10) {

        // Generate random position
        float x = distX(rng);
        float y = distY(rng);

        // Spawn enemy
        spawnEntity(GeneralEntity::EntityType::Enemy, x, y, frame);
        lastSpawnTime = now;
        }
}

size_t GameState::getEntityCount() const {
    return entities.size();
}

extern "C" GameState* create_game(void* server) {
    return new GameState(static_cast<RType::Server*>(server));
}
