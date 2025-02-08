#ifndef GAME_STATE_HPP
#define GAME_STATE_HPP

#include "AGame.hpp"
#include "Registry.hpp"
#include "PlayerAction.hpp"
#include "EngineFrame.hpp"
#include "GeneralEntity.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <mutex>
#include <random>
#include <memory> 
#include <chrono>

namespace RType {
    class Server;
}

class GameState : public AGame {
    public:
        GameState(RType::Server* server);
        ~GameState();

        void run(int numPlayers);

        // Implement player action management functions
        void addPlayerAction(int playerId, int actionId) override;
        void processPlayerActions(EngineFrame &frame);
        void deletePlayerAction();
        const std::vector<PlayerAction>& getPlayerActions() const;

        std::pair<float, float> getEntityPosition(int entityId) const override;
        std::map<int, GeneralEntity>& getEntities() override;
        std::map<int, EngineFrame>& getEngineFrames() override;

        // Implement entity spawn and delete management functions
        void spawnEntity(GeneralEntity::EntityType type, float x, float y, EngineFrame &frame);
        void killEntity(int entityId, EngineFrame &frame);


        //Implement generic Game Engine function to create a game from these
        void registerComponents();
        void moveBullets(EngineFrame &frame);
        void moveEnemies(EngineFrame &frame);
        void checkCollisions(GeneralEntity::EntityType typeA, GeneralEntity::EntityType typeB, float collisionThreshold, EngineFrame &frame);

        //GameState methods
        void spawnEnemiesRandomly(EngineFrame &frame);
        void initializeplayers(int numPlayers, EngineFrame &frame);
        void handlePlayerMove(int playerId, int actionId);
        bool isBossSpawned() const;
        bool areEnemiesCleared() const;
        int countPlayers() const;
        void startNextWave(EngineFrame &frame);

        int currentWave;
        size_t getEntityCount() const;
        void update(EngineFrame &frame);

    private:
    //old AGame variables
    int id_to_set = 0;
    std::vector<PlayerAction> playerActions;
    std::map<int, GeneralEntity> entities;
    std::map<int, EngineFrame> engineFrames;
    Registry registry;
    RType::Server* m_server;
    std::mutex playerActionsMutex;

    //GameState Variables
    std::mt19937 rng;
    std::uniform_real_distribution<float> distX;
    std::uniform_real_distribution<float> distY;
    std::uniform_int_distribution<int> distTime;
    int enemiesPerWave;
    std::chrono::steady_clock::time_point lastSpawnTime;
    sf::Clock frameClock;
    const sf::Time frameDuration = sf::milliseconds(10);
    int nextEnemyId;
    int nextBossId;
};

#endif // GAME_STATE_HPP