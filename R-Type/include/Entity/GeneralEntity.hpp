#ifndef GENERAL_ENTITY_HPP
#define GENERAL_ENTITY_HPP

#include "Registry.hpp"
#include <string>

class GeneralEntity {
public:
    enum class EntityType {
        Player,
        Enemy,
        Bullet,
        Boss,
        EnemyBullet,
        Ball
    };

    GeneralEntity(Registry& registry, EntityType type, float x, float y);
    ~GeneralEntity();

    void move(float x, float y);
    Registry::Entity getEntity() const;

    const Registry& getRegistry() const;
    Registry& getRegistry();
    void setRegistry(const Registry& newRegistry);

    EntityType getType() const;
    void setType(EntityType type);

    int getNumberOfLives() const;
    void setNumberOfLives(int newNumberOfLives);

private:
    void addComponents(EntityType type, float x, float y);

    Registry registry;
    Registry::Entity entity;
    EntityType type;
    int numberOfLives = 1;
};

#endif // GENERAL_ENTITY_HPP
