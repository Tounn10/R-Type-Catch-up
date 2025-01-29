/*
** EPITECH PROJECT, 2025
** R-Type [WSL: Ubuntu]
** File description:
** AGame
*/

#include "GameState.hpp"
#include "Packet.hpp"
#include "PacketType.hpp"
#include "Server.hpp"
#include "AGame.hpp"
#include <algorithm>
#include <iostream>

AGame::AGame(RType::Server* server) : m_server(server)
{
    registerComponents();
}

AGame::~AGame()
{
    playerActions.clear();
    entities.clear();
}

std::map<int, GeneralEntity>& AGame::getEntities() {
    return entities;
}

std::map<int, EngineFrame>& AGame::getEngineFrames() {
    return engineFrames;
}

void AGame::registerComponents()
{
    registry.register_component<Position>();
    registry.register_component<Velocity>();
    registry.register_component<Drawable>();
    registry.register_component<Controllable>();
    registry.register_component<Collidable>();
    registry.register_component<Projectile>();
}

void AGame::addPlayerAction(int playerId, int actionId) {
    std::cout << "Player " << playerId << " performed action " << actionId << std::endl;
    std::lock_guard<std::mutex> lock(playerActionsMutex);
    playerActions.emplace_back(playerId, actionId);
}

void AGame::processPlayerActions(EngineFrame &frame) {
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

void AGame::deletePlayerAction() { //deletes all process elements from the vector of player actions and resizes vector
    playerActions.erase(
        std::remove_if(playerActions.begin(), playerActions.end(),
            [](const PlayerAction& action) { return action.getProcessed(); }),
        playerActions.end());
}

const std::vector<PlayerAction>& AGame::getPlayerActions() const {
    return playerActions;
}

std::pair<float, float> AGame::getEntityPosition(int entityId) const
{
    auto it = entities.find(entityId);
    if (it == entities.end()) {
        throw std::out_of_range("Invalid entity ID");
    }

    const auto& positionComponent = it->second.getRegistry().get_components<Position>()[it->second.getEntity()];
    return {positionComponent->x, positionComponent->y};
}

void AGame::spawnEntity(GeneralEntity::EntityType type, float x, float y, EngineFrame &frame) {
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
    id_to_set++;
}

void AGame::killEntity(int entityId, EngineFrame &frame)
{
    auto it = entities.find(entityId);
    if (it != entities.end()) {
        it->second.getRegistry().kill_entity(it->second.getEntity());
        entities.erase(it);
        std::string data = std::to_string(entityId) + ";-1;-1/";
        frame.frameInfos += m_server->createPacket(Network::PacketType::DELETE, data);
    }
}

void AGame::moveBullets(EngineFrame &frame) {
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

void AGame::checkCollisions(GeneralEntity::EntityType typeA, GeneralEntity::EntityType typeB, float collisionThreshold, EngineFrame &frame) {
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

void AGame::moveEnemies(EngineFrame &frame) {
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
