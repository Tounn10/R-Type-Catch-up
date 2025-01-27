#include "Server.hpp"
#include "GameState.hpp"
#include "AGame.hpp"
#include "CollisionSystem.hpp"
#include <algorithm>
#include <iostream>
#include <thread>

GameState::GameState(RType::Server* server)
    : AGame(server), rng(std::random_device()()), distX(0.0f, 800.0f), distY(0.0f, 600.0f),
      distTime(1000, 5000), currentWave(0), enemiesPerWave(5), m_server(server), nextEnemyId(0), nextBossId(0) {}

void GameState::initializeplayers(int numPlayers) {
    for (int i = 0; i < numPlayers; ++i) {
        spawnPlayer(i, 100.0f * (i + 1.0f), 100.0f);
    }
}

void GameState::update() {
    registry.run_systems();
    processPlayerActions();
    moveBullets();
    //checkBulletEnemyCollisions();
    //moveEnemies();
}

void GameState::run(int numPlayers) {
    initializeplayers(numPlayers);
    while (true) {
        // Update game state
        update();

         //Check if all enemies are cleared and start the next wave or spawn the boss
        if (areEnemiesCleared()) {
             if (isBossSpawned()) {
                 std::cout << "Boss defeated! Game over." << std::endl;
                 break;
             } else if (currentWave >= 3) {
                 spawnBoss(nextBossId++, 400.0f, 300.0f); // Spawn boss at the center of the screen
             } else {
                 startNextWave();
             }
        } else {
            spawnEnemiesRandomly();
        }

        //Sleep for a short duration to simulate frame time
        //Actually do a clock to make sure that frames aren't computed too fast (same clock as client)
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
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
    auto it = players.find(playerId);
    if (it != players.end()) {
        it->second.move(x, y);
    } else {
        std::cerr << "[ERROR] Player ID " << playerId << " not found." << std::endl;
    }
}

bool GameState::isBossSpawned() const {
    return !bosses.empty();
}

bool GameState::areEnemiesCleared() const {
    return enemies.empty();
}

void GameState::startNextWave() {
    currentWave++;
    enemiesPerWave += 5; // Increase the number of enemies per wave
    for (int i = 0; i < enemiesPerWave; ++i) {
        spawnEnemy(nextEnemyId++, distX(rng), distY(rng));
    }
}

void GameState::spawnEnemiesRandomly() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSpawnTime).count();

    if (elapsed > distTime(rng) && enemies.size() < 10) {
        float x = distX(rng);
        float y = distY(rng);
        spawnEnemy(nextEnemyId++, x, y);
        lastSpawnTime = now;
    }
}

size_t GameState::getPlayerCount() const {
    return players.size();
}

size_t GameState::getEnemiesCount() const {
    return enemies.size();
}

size_t GameState::getBulletsCount() const {
    return bullets.size();
}

size_t GameState::getBossCount() const {
    return bosses.size();
}
