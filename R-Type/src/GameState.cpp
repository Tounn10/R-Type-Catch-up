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
#include <random>
#include <thread>

GameState::GameState(RType::Server* server) : m_server(server) {
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
    std::lock_guard<std::mutex> lock(playerActionsMutex);
    playerActions.emplace_back(playerId, actionId);
}

void GameState::processPlayerActions(EngineFrame &frame) {
    for (auto& action : playerActions) {
        int playerId = action.getId();
        int actionId = action.getActionId();

        if (actionId > 0 && actionId < 5) {
            handlePlayerMove(playerId, actionId);
            action.setProcessed(true);
        } else if (actionId == 5) {
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

void GameState::deletePlayerAction() {
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
    case GeneralEntity::EntityType::EnemyBullet:
        packetType = Network::PacketType::CREATE_ENEMY_BULLET;
        break;
    default:
        std::cerr << "Error: Unsupported entity type for spawning." << std::endl;
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
    const float maxX = 1500;
    const float bulletSpeed = 3.0f;

    std::map <int, GeneralEntity> temp_entities = entities;
    for (auto& [id, entity] : temp_entities) {
        if (entity.getType() == GeneralEntity::EntityType::Bullet) {
            auto [x, y] = getEntityPosition(id);
            float newX = x + bulletSpeed;
            if (newX > maxX) {
                killEntity(id, frame);
            } else {
                entities.find(id)->second.move(bulletSpeed, 0.0f);
            }
        }
    }
}

void GameState::moveEnemyBullets(EngineFrame &frame) {
    const float minX = 0.0f;
    const float bulletSpeed = -3.0f;

    std::map<int, GeneralEntity> temp_entities = entities;
    for (auto& [id, entity] : temp_entities) {
        if (entity.getType() == GeneralEntity::EntityType::EnemyBullet) {
            auto [x, y] = getEntityPosition(id);
            float newX = x + bulletSpeed;
            if (newX < minX) {
                killEntity(id, frame);
            } else {
                entities.find(id)->second.move(bulletSpeed, 0.0f);
            }
        }
    }
}

void GameState::checkCollisions(GeneralEntity::EntityType typeA, GeneralEntity::EntityType typeB, float thresholdX, float thresholdY, EngineFrame &frame) {
    auto tempEntities = entities; // Copy to avoid iterator invalidation

    for (auto& [idA, entityA] : tempEntities) {
        if (entityA.getType() != typeA) continue;
        auto [posXA, posYA] = getEntityPosition(idA);

        for (auto& [idB, entityB] : tempEntities) {
            if (entityB.getType() != typeB || idA == idB) continue;

            auto [posXB, posYB] = getEntityPosition(idB);

            if (std::abs(posXA - posXB) < thresholdX && std::abs(posYA - posYB) < thresholdY) {
                if (entityA.getNumberOfLives() == 1) {
                    killEntity(idA, frame);
                } else {
                    entities.find(idA)->second.setNumberOfLives(entityA.getNumberOfLives() - 1);
                }
                if (entityB.getNumberOfLives() == 1) {
                    killEntity(idB, frame);
                } else {
                    entities.find(idB)->second.setNumberOfLives(entityB.getNumberOfLives() - 1);
                }
                return;
            }
        }
    }
}

void GameState::moveEnemies(EngineFrame &frame) {
    static int direction = 0;
    static float distanceMoved = 0.0f;
    const float moveDistance = 2.0f;
    const float switchThreshold = 25.0f;

    static sf::Clock moveClock;
    const sf::Time moveInterval = sf::milliseconds(100);

    if (moveClock.getElapsedTime() >= moveInterval)
    {
        moveClock.restart();
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
                distanceMoved += moveDistance;

                if (distanceMoved >= switchThreshold) {
                    direction = (direction + 1) % 4;
                    distanceMoved = 0.0f;
                }

                if (countEnemyBullets() < maxEnemyBullets && rand() % 100 < 0.0) {
                    auto[x, y] = getEntityPosition(id);
                    spawnEntity(GeneralEntity::EntityType::EnemyBullet, x - 50.0f, y - 25.0f, frame);
                }
            }
        }
    }
}

void GameState::moveBoss(EngineFrame &frame) {
    static int direction = 0;
    static float distanceMoved = 0.0f;
    const float moveDistance = 2.0f;
    const float switchThreshold = 50.0f;

    static sf::Clock moveClock;
    const sf::Time moveInterval = sf::milliseconds(100);

    if (moveClock.getElapsedTime() >= moveInterval) {
        moveClock.restart();

        std::map<int, GeneralEntity> temp_entities = entities;
        for (auto& [id, entity] : temp_entities) {
            if (entity.getType() == GeneralEntity::EntityType::Boss) {
                float x = 0.0f, y = 0.0f;

                direction = rand() % 4;
                switch (direction) {
                case 0: y = -moveDistance; break; // Up
                case 1: x = -moveDistance; break; // Left
                case 2: y = moveDistance; break;  // Down
                case 3: x = moveDistance; break;  // Right
                }

                entities.find(id)->second.move(x, y);
                distanceMoved += moveDistance;

                if (distanceMoved >= switchThreshold) {
                    direction = rand() % 4;
                    distanceMoved = 0.0f;
                }

                if (countEnemyBullets() < (maxEnemyBullets + 5) && rand() % 100 < 1.0) {
                    auto [x, y] = getEntityPosition(id);
                    spawnEntity(GeneralEntity::EntityType::EnemyBullet, x - 50.0f, y - 25.0f, frame);
                }
            }
        }
    }
}

void GameState::initializeplayers(int numPlayers, EngineFrame &frame) {
    for (int i = countPlayers(); i < numPlayers; ++i) {
        spawnEntity(GeneralEntity::EntityType::Player, 100.0f * (i + 1.0f), 100.0f, frame);
        frame.frameInfos += m_server->createPacket(Network::PacketType::CREATE_BACKGROUND, "-100;O;O/"); // Check if background is created
        frame.frameInfos += m_server->createPacket(Network::PacketType::IMPORTANT_PACKET, "-1;-1;-1/");
    }
}

void GameState::update(EngineFrame &frame) {
    registry.run_systems();
    initializeplayers(m_server->getClients().size(), frame);
    processPlayerActions(frame);

    if (areEnemiesCleared()) {
        if (currentWave < numberOfWaves) {
            spawnEnemiesRandomly(frame);
        } else if (currentBoss < numberOfBoss) {
            spawnBossRandomly(frame);
        }
    }
    moveBullets(frame);
    checkCollisions(GeneralEntity::EntityType::Bullet, GeneralEntity::EntityType::Enemy, 20.0f, 40.0f, frame);
    moveEnemies(frame);
    moveEnemyBullets(frame);
    checkCollisions(GeneralEntity::EntityType::EnemyBullet, GeneralEntity::EntityType::Player, 30.0f, 50.0f, frame);
    moveBoss(frame);
    checkCollisions(GeneralEntity::EntityType::Bullet, GeneralEntity::EntityType::Boss, 50.0f, 50.0f, frame);
    //Handle lose condition if player got hit
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
    float moveDistance = 3.0f;
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

int GameState::countEnemyBullets() const {
    return std::count_if(entities.begin(), entities.end(), [](const auto& pair) {
        return pair.second.getType() == GeneralEntity::EntityType::EnemyBullet;
    });
}

bool GameState::areEnemiesCleared() const {
    return std::none_of(entities.begin(), entities.end(), [](const auto& pair) {
        return pair.second.getType() == GeneralEntity::EntityType::Enemy;
    });
}

float randomFloat(float min, float max) {
    return min + static_cast<float>(std::rand()) / (RAND_MAX / (max - min));
}

void GameState::spawnEnemiesRandomly(EngineFrame &frame) {
    for (int i = 0; i < enemiesPerWave; ++i) {

        float x = randomFloat(1280 - 300, 1280 - 50);
        float y = randomFloat(50, 720 - 50);

        spawnEntity(GeneralEntity::EntityType::Enemy, x, y, frame);
    }
    currentWave++;
}

void GameState::spawnBossRandomly(EngineFrame &frame) {
    for (int i = 0; i < numberOfBoss; ++i) {

        float x = randomFloat(1280 - 300, 1280 - 100);
        float y = randomFloat(50, 720 - 50);

        spawnEntity(GeneralEntity::EntityType::Boss, x, y, frame);
    }
    currentBoss++;
}


size_t GameState::getEntityCount() const {
    return entities.size();
}

extern "C" AGame* create_game(void* server) {
    return new GameState(static_cast<RType::Server*>(server));
}
