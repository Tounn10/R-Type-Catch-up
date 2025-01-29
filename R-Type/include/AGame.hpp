/*
** EPITECH PROJECT, 2025
** R-Type [WSL: Ubuntu]
** File description:
** AGame
*/

#ifndef AGAME_HPP
#define AGAME_HPP

#include "IGame.hpp"
#include "Registry.hpp"
#include "Position.hpp"
#include "Velocity.hpp"
#include "Drawable.hpp"
#include "Controllable.hpp"
#include "Collidable.hpp"
#include "Projectile.hpp"
#include "PlayerAction.hpp"
#include "GeneralEntity.hpp"
#include "EngineFrame.hpp"
#include "Drawable.hpp"
#include <vector>
#include <mutex>

namespace RType {
    class Server;
}

class AGame : public IGame {
    protected:
        int id_to_set = 0;
        std::vector<PlayerAction> playerActions; // Shared player action system
        std::map<int, GeneralEntity> entities;
        std::map<int, EngineFrame> engineFrames;
        Registry registry;
        RType::Server* m_server;
        std::mutex playerActionsMutex;

    public:
        AGame(RType::Server* server);
        virtual ~AGame();

        // Implement player action management functions
        void addPlayerAction(int playerId, int actionId) override;
        void processPlayerActions(EngineFrame &frame) override;
        void deletePlayerAction() override;
        const std::vector<PlayerAction>& getPlayerActions() const override;

        std::pair<float, float> getEntityPosition(int entityId) const override;
        std::map<int, GeneralEntity>& getEntities() override;
        std::map<int, EngineFrame>& getEngineFrames() override;

        // Implement entity spawn and delete management functions
        void spawnEntity(GeneralEntity::EntityType type, float x, float y, EngineFrame &frame) override;
        void killEntity(int entityId, EngineFrame &frame) override;

        void registerComponents();
        void moveBullets(EngineFrame &frame) override;
        void moveEnemies(EngineFrame &frame) override;
        void checkCollisions(GeneralEntity::EntityType typeA, GeneralEntity::EntityType typeB, float collisionThreshold, EngineFrame &frame) override;
};

#endif // AGAME_HPP
