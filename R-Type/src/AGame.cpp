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
    players.clear();
    enemies.clear();
    bullets.clear();
    bosses.clear();
}

std::map<int, Player>& AGame::getPlayers() {
    return players;
}

std::map<int, Enemy>& AGame::getEnemies() {
    return enemies;
}

std::map<int, Bullet>& AGame::getBullets() {
    return bullets;
}

std::map<int, Boss>& AGame::getBosses() {
    return bosses;
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

void AGame::processPlayerActions() {
    for (auto& action : playerActions) {
        int playerId = action.getId();
        int actionId = action.getActionId();

        if (actionId > 0 && actionId < 5) { // Change by real action ID defined in server
            handlePlayerMove(playerId, actionId);
            action.setProcessed(true);
        } else if (actionId == 5) { // Change by real action ID defined in server
            spawnBullet(playerId);
            action.setProcessed(true);
        }
        // Handle other actions or ignore unknown action IDs
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

std::pair<float, float> AGame::getPlayerPosition(int playerId) const {
    auto it = players.find(playerId);
    if (it == players.end()) {
        throw std::out_of_range("Invalid player ID");
    }

    const auto& positionComponent = it->second.getRegistry().get_components<Position>()[it->second.getEntity()];
    return {positionComponent->x, positionComponent->y};
}

std::pair<float, float> AGame::getEnemyPosition(int enemyId) const {
    auto it = enemies.find(enemyId);
    if (it == enemies.end()) {
        throw std::out_of_range("Invalid enemy ID");
    }

    const auto& positionComponent = it->second.getRegistry().get_components<Position>()[it->second.getEntity()];
    return {positionComponent->x, positionComponent->y};
}

std::pair<float, float> AGame::getBulletPosition(int bulletId) const {
    auto it = bullets.find(bulletId);
    if (it == bullets.end()) {
        throw std::out_of_range("Invalid bullet ID");
    }

    const auto& positionComponent = it->second.getRegistry().get_components<Position>()[it->second.getEntity()];
    return {positionComponent->x, positionComponent->y};
}

std::pair<float, float> AGame::getBossPosition(int bossId) const {
    auto it = bosses.find(bossId);
    if (it == bosses.end()) {
        throw std::out_of_range("Invalid boss ID");
    }

    const auto& positionComponent = it->second.getRegistry().get_components<Position>()[it->second.getEntity()];
    return {positionComponent->x, positionComponent->y};
}

void AGame::spawnEnemy(int enemyId, float x, float y) {
    enemies.emplace(enemyId, Enemy(registry, x, y));

    std::string data = std::to_string(enemyId + 500) + ";" + std::to_string(x) + ";" + std::to_string(y);
    m_server->Broadcast(m_server->createPacket(Network::PacketType::CREATE_ENEMY, data));
}

void AGame::spawnBoss(int bossId, float x, float y) {
    bosses.emplace(bossId, Boss(registry, x, y));

    std::string data = std::to_string(bossId + 900) + ";" + std::to_string(x) + ";" + std::to_string(y);
    m_server->Broadcast(m_server->createPacket(Network::PacketType::CREATE_BOSS, data));
}

void AGame::spawnPlayer(int playerId, float x, float y) {
    if (playerId >= 0 && playerId < 4) {
        players.emplace(playerId, Player(registry, x, y));

        std::string data = std::to_string(playerId) + ";" + std::to_string(x) + ";" + std::to_string(y);
        std::cout << "Player " << playerId << " spawned at " << x << ", " << y << std::endl;
        m_server->Broadcast(m_server->createPacket(Network::PacketType::CREATE_PLAYER, data));
    }
}

void AGame::spawnBullet(int playerId) {
    auto it = players.find(playerId);
    if (it != players.end()) {
        auto entity = it->second.getEntity();
        if (it->second.getRegistry().has_component<Position>(entity)) {
            const auto& position = it->second.getRegistry().get_components<Position>()[entity];
            int bulletId = bullets.size(); // Generate a new bullet ID
            bullets.emplace(bulletId, Bullet(registry, position->x + 50.0f, position->y + 25.0f, 1.0f));

            std::string data = std::to_string(bulletId + 200) + ";" + std::to_string(position->x + 50.0f) + ";" + std::to_string(position->y + 25.0f);
            m_server->Broadcast(m_server->createPacket(Network::PacketType::CREATE_BULLET, data));
        } else {
            std::cerr << "Error: Player " << playerId << " does not have a Position component." << std::endl;
        }
    }
}

void AGame::spawnEnemyBullet(int enemyId) {
    auto it = enemies.find(enemyId);
    if (it != enemies.end()) {
        auto entity = it->second.getEntity();
        if (it->second.getRegistry().has_component<Position>(entity)) {
            const auto& position = it->second.getRegistry().get_components<Position>()[entity];
            int bulletId = bullets.size(); // Generate a new bullet ID
            bullets.emplace(bulletId, Bullet(registry, position->x - 50.0f, position->y + 25.0f, -1.0f));

            std::string data = std::to_string(bulletId + 200) + ";" + std::to_string(position->x - 50.0f) + ";" + std::to_string(position->y + 25.0f);
            m_server->Broadcast(m_server->createPacket(Network::PacketType::CREATE_BULLET, data));
        } else {
            std::cerr << "Error: Enemy " << enemyId << " does not have a Position component." << std::endl;
        }
    }
}

void AGame::killPlayers(int entityId) {
    auto it = players.find(entityId);
    if (it != players.end()) {
        it->second.getRegistry().kill_entity(it->second.getEntity());
        players.erase(it);
        std::string data = std::to_string(entityId) + ";-1;-1";
        m_server->Broadcast(m_server->createPacket(Network::PacketType::DELETE, data));
    }
}

void AGame::killEnemies(int entityId) {
    auto it = enemies.find(entityId);
    if (it != enemies.end()) {
        it->second.getRegistry().kill_entity(it->second.getEntity());
        enemies.erase(it);
        std::string data = std::to_string(entityId + 500) + ";-1;-1";
        m_server->Broadcast(m_server->createPacket(Network::PacketType::DELETE, data));
    }
}

void AGame::killBullets(int entityId) {
    auto it = bullets.find(entityId);
    if (it != bullets.end()) {
        it->second.getRegistry().kill_entity(it->second.getEntity());
        bullets.erase(it);
        std::string data = std::to_string(entityId + 200) + ";-1;-1";
        m_server->Broadcast(m_server->createPacket(Network::PacketType::DELETE, data));
    }
}

void AGame::killBosses(int entityId) {
    auto it = bosses.find(entityId);
    if (it != bosses.end()) {
        it->second.getRegistry().kill_entity(it->second.getEntity());
        bosses.erase(it);
        std::string data = std::to_string(entityId + 900) + ";-1;-1";
        m_server->Broadcast(m_server->createPacket(Network::PacketType::DELETE, data));
    }
}

void AGame::killEntity(int entityId) {
    killPlayers(entityId);
    killEnemies(entityId);
    killBullets(entityId);
    killBosses(entityId);
}

void AGame::moveBullets() {
    const float maxX = 800.0f;
    
    std::map <int, Bullet> temp_bullets = bullets;
    for (auto& [id, bullet] : temp_bullets) {
        auto [x, y] = getBulletPosition(id);
        float newX = x + 1.0f;
        if (newX > maxX) {
            killBullets(id);
        } else {
            bullets.find(id)->second.move(1.0f, 0.0f);
        }
    }
}

void AGame::checkBulletEnemyCollisions() {
    std::map<int, Bullet> temp_bullets = bullets;
    std::map<int, Enemy> temp_enemies = enemies;

    for (const auto& [bulletId, bullet] : temp_bullets) {
        auto [bulletX, bulletY] = getBulletPosition(bulletId);
        for (const auto& [enemyId, enemy] : temp_enemies) {
            auto [enemyX, enemyY] = getEnemyPosition(enemyId);
            float distance = std::sqrt(std::pow(enemyX - bulletX, 2) + std::pow(enemyY - bulletY, 2));
            float collisionThreshold = 30.0f;

            if (distance < collisionThreshold) {
                killBullets(bulletId);
                killEnemies(enemyId);
            }
        }
    }
}

void AGame::moveEnemies() {
    static int direction = 0;
    const float moveDistance = 1.0f;

    std::map<int, Enemy> temp_enemies = enemies;
    for (auto& [id, enemy] : temp_enemies) {
        float x = 0.0f;
        float y = 0.0f;

        switch (direction) {
            case 0: y = -moveDistance; break;
            case 1: x = -moveDistance; break;
            case 2: y = moveDistance; break;
            case 3: x = moveDistance; break;
        }

        //enemies.find(id)->second.move(x, y); Works but heavy in packets will implement it with new system

        if (rand() % 100 < 0.1) {
            //spawnEnemyBullet(id); Check for another type of bullets to avoid killing enneimes by their own bullets
        }
    }

    direction = (direction + 1) % 4;
}
