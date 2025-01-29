#ifndef GAME_STATE_HPP
#define GAME_STATE_HPP

#include "AGame.hpp"
#include "Registry.hpp"
#include <random>
#include <memory> 
#include <chrono>

class GameState : public AGame {
public:
    GameState(RType::Server* server);

    void initializeplayers(int numPlayers, EngineFrame &frame);
    void update(EngineFrame &frame) override;
    void handlePlayerMove(int playerId, int actionId) override;
    bool isBossSpawned() const;
    bool areEnemiesCleared() const;
    void startNextWave(EngineFrame &frame);
    void run(int numPlayers);

    int currentWave;
    size_t getEntityCount() const override;

private:
    std::mt19937 rng;
    std::uniform_real_distribution<float> distX;
    std::uniform_real_distribution<float> distY;
    std::uniform_int_distribution<int> distTime;
    int enemiesPerWave;
    std::chrono::steady_clock::time_point lastSpawnTime;
    sf::Clock frameClock;
    const sf::Time frameDuration = sf::milliseconds(10);

    void spawnEnemiesRandomly(EngineFrame &frame);
    RType::Server* m_server; // Pointer to RType::Server
    int nextEnemyId;
    int nextBossId;
};

#endif // GAME_STATE_HPP