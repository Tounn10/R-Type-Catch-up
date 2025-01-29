/*
** EPITECH PROJECT, 2025
** R-Type [WSL: Ubuntu]
** File description:
** IGame
*/

#ifndef IGAME_HPP
#define IGAME_HPP

#include <vector>
#include <utility>
#include <cstddef>
#include "PlayerAction.hpp"
#include "EngineFrame.hpp"
#include "GeneralEntity.hpp"
#include <map>

class IGame {
public:
    virtual ~IGame() = default;

    // Pure virtual functions to be implemented by noe in GameState class
    virtual void update(EngineFrame &frame) = 0;
    virtual void handlePlayerMove(int playerId, int actionId) = 0;
    virtual void killEntity(int entityId, EngineFrame &frame) = 0;
    virtual void spawnEntity(GeneralEntity::EntityType type, float x, float y, EngineFrame &frame) = 0;
    virtual size_t getEntityCount() const = 0;
    virtual std::pair<float, float> getEntityPosition(int entityId) const = 0;
    virtual std::map<int, EngineFrame>& getEngineFrames() = 0;
    virtual std::map<int, GeneralEntity>& getEntities() = 0;
    virtual void moveBullets(EngineFrame &frame) = 0;
    virtual void moveEnemies(EngineFrame &frame) = 0;
    virtual void checkCollisions(GeneralEntity::EntityType typeA, GeneralEntity::EntityType typeB, float collisionThreshold, EngineFrame &frame) = 0;

    // Functions for managing player actions
    virtual void addPlayerAction(int playerId, int actionId) = 0;
    virtual void processPlayerActions(EngineFrame &frame) = 0;
    virtual void deletePlayerAction() = 0;
    virtual const std::vector<PlayerAction>& getPlayerActions() const = 0;
};

#endif // IGAME_HPP
