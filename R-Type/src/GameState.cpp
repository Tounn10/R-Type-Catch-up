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
        spawnEntity(GeneralEntity::EntityType::Player, 100.0f * (i + 1.0f), 100.0f);
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
    spawnEnemiesRandomly();
    int frameId = 0;
    int last_frame_sent = 0;

    while (true) {
        if (frameClock.getElapsedTime() >= frameDuration)
        {
            EngineFrame new_frame;
            frameClock.restart();
            update();
            //Check if all enemies are cleared and start the next wave or spawn the boss
            if (areEnemiesCleared()) {
                if (isBossSpawned()) {
                    std::cout << "Boss defeated! Game over." << std::endl;
                    break;
                } else if (currentWave >= 3) {
                    spawnEntity(GeneralEntity::EntityType::Boss, 400.0f, 300.0f); // Spawn boss at the center of the screen
                } else {
                    startNextWave();
                }
            } else {
                spawnEnemiesRandomly();
            }
            m_server->server_mutex.lock();
            std::cout << "Frame ID: " << frameId++ << std::endl;
            engineFrames.emplace(frameId++, new_frame);
            m_server->server_mutex.unlock();
            //if (engineFrames.size() > 60) { This needs to go to the server as well as the clock
            //    m_server->SendFrame(engineFrames.at(last_frame_sent++));
            //}
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

void GameState::startNextWave() {
    currentWave++;
    enemiesPerWave += 5; // Increase the number of enemies per wave
    for (int i = 0; i < enemiesPerWave; ++i) {
        spawnEntity(GeneralEntity::EntityType::Enemy, distX(rng), distY(rng));
    }
}

void GameState::spawnEnemiesRandomly() {
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
        spawnEntity(GeneralEntity::EntityType::Enemy, x, y);
        lastSpawnTime = now;
        }
}

size_t GameState::getEntityCount() const {
    return entities.size();
}
