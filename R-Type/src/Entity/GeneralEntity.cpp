#include "GeneralEntity.hpp"
#include "Position.hpp"
#include "Velocity.hpp"
#include "Drawable.hpp"
#include "Collidable.hpp"
#include "Controllable.hpp"
#include "Projectile.hpp"
#include <iostream>

GeneralEntity::GeneralEntity(Registry& registry, EntityType type, float x, float y) : registry(registry), type(type) {
    entity = this->registry.spawn_entity();
    addComponents(type, x, y);
}

GeneralEntity::~GeneralEntity() {
    if (registry.entity_exists(entity))
        registry.kill_entity(entity);
}

void GeneralEntity::addComponents(EntityType type, float x, float y) {
    this->registry.add_component<Position>(entity, {x, y});

    switch (type) {
        case EntityType::Player:
            this->registry.add_component<Velocity>(entity, {0.0f, 0.0f});
            this->registry.add_component<Drawable>(entity, {sf::RectangleShape(sf::Vector2f(50.0f, 50.0f))});
            this->registry.add_component<Controllable>(entity, {});
            this->registry.add_component<Collidable>(entity, {true});
            break;
        case EntityType::Enemy:
            this->registry.add_component<Drawable>(entity, {sf::RectangleShape(sf::Vector2f(50.0f, 50.0f))});
            this->registry.add_component<Collidable>(entity, {true});
            break;
        case EntityType::Bullet:
            this->registry.add_component<Projectile>(entity, {1.0f});
            this->registry.add_component<Drawable>(entity, {sf::RectangleShape(sf::Vector2f(5.0f, 5.0f))});
            this->registry.add_component<Collidable>(entity, {true});
            break;
        case EntityType::Ball:
            this->registry.add_component<Projectile>(entity, {1.0f});
            this->registry.add_component<Drawable>(entity, {sf::RectangleShape(sf::Vector2f(5.0f, 5.0f))});
            this->registry.add_component<Collidable>(entity, {true});
            break;
        case EntityType::Boss:
            this->registry.add_component<Velocity>(entity, {0.0f, 0.0f});
            this->registry.add_component<Drawable>(entity, {sf::RectangleShape(sf::Vector2f(100.0f, 100.0f))});
            this->registry.add_component<Collidable>(entity, {true});
            this->numberOfLives = 3;
            break;
        case EntityType::EnemyBullet:
            this->registry.add_component<Projectile>(entity, {1.0f});
            this->registry.add_component<Drawable>(entity, {sf::RectangleShape(sf::Vector2f(5.0f, 5.0f))});
            this->registry.add_component<Collidable>(entity, {true});
            break;
    }
}

void GeneralEntity::move(float x, float y) {
    if (registry.has_component<Position>(entity)) {
        auto& pos = registry.get_components<Position>()[entity];
        pos->x += x;
        pos->y += y;
    } else {
        std::cerr << "Error: Entity does not have a Position component." << std::endl;
    }
}

Registry::Entity GeneralEntity::getEntity() const {
    return entity;
}

const Registry& GeneralEntity::getRegistry() const {
    return registry;
}

Registry& GeneralEntity::getRegistry() {
    return registry;
}

void GeneralEntity::setRegistry(const Registry& newRegistry) {
    registry = newRegistry;
}

GeneralEntity::EntityType GeneralEntity::getType() const {
    return type;
}

void GeneralEntity::setType(EntityType newType) {
    type = newType;
}

int GeneralEntity::getNumberOfLives() const {
    return this->numberOfLives;
}

void GeneralEntity::setNumberOfLives(int newNumberOfLives) {
    this->numberOfLives = newNumberOfLives;
}
