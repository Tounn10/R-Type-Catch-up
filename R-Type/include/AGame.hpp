/*
** EPITECH PROJECT, 2025
** R-Type [WSL: Ubuntu]
** File description:
** AGame
*/

#ifndef AGAME_HPP
#define AGAME_HPP

#include "IGame.hpp"
#include "Player.hpp"
#include "Enemy.hpp"
#include "Bullet.hpp"
#include "Registry.hpp"
#include "Boss.hpp"
#include "Registry.hpp"
#include "Position.hpp"
#include "Velocity.hpp"
#include "Drawable.hpp"
#include "Controllable.hpp"
#include "Collidable.hpp"
#include "Projectile.hpp"
#include "PlayerAction.hpp"
#include "Position.hpp"
#include "Drawable.hpp"
#include "Collidable.hpp"
#include "Controllable.hpp"
#include "Projectile.hpp"
#include "Velocity.hpp"
#include "Registry.hpp"
#include <vector>
#include <mutex>

namespace RType {
    class Server;
}

class AGame : public IGame {
    protected:
        std::vector<PlayerAction> playerActions; // Shared player action system
        std::map<int, Player> players;
        std::map<int, Enemy> enemies;
        std::map<int, Bullet> bullets;
        std::map<int, Boss> bosses;
        Registry registry;
        RType::Server* m_server;
        std::mutex playerActionsMutex;

    public:
        AGame(RType::Server* server);
        virtual ~AGame();

        // Implement player action management functions
        void addPlayerAction(int playerId, int actionId) override;
        void processPlayerActions() override;
        void deletePlayerAction() override;
        const std::vector<PlayerAction>& getPlayerActions() const override;

        //Getter functions for player, bullet and enemy positions for server to build package to send to client
        std::pair<float, float> getPlayerPosition(int playerId) const override;
        std::pair<float, float> getBulletPosition(int bulletId) const override;
        std::pair<float, float> getEnemyPosition(int enemyId) const override;
        std::pair<float, float> getBossPosition(int enemyId) const override;

        // Public getter methods for maps of players, enemies, bullets and bosses
        std::map<int, Player>& getPlayers() override;
        std::map<int, Enemy>& getEnemies() override;
        std::map<int, Bullet>& getBullets() override;
        std::map<int, Boss>& getBosses() override;

        // Implement entity spawn and delete management functions
        void spawnEnemy(int enemyId, float x, float y) override;
        void spawnBoss(int boosId, float x, float y) override;
        void spawnPlayer(int playerId, float x, float y) override;
        void spawnBullet(int playerId) override;
        void spawnEnemyBullet(int enemyId) override;
        void killBosses(int entityId) override;
        void killBullets(int entityId) override;
        void killEnemies(int entityId) override;
        void killPlayers(int entityId) override;
        void killEntity(int entityId) override;

        void registerComponents();
        void moveBullets() override;
        void moveEnemies() override;
        void checkBulletEnemyCollisions() override;
};

#endif // AGAME_HPP
