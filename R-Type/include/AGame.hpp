/*
** EPITECH PROJECT, 2025
** R-Type [WSL: Ubuntu]
** File description:
** AGame
*/

#ifndef AGAME_HPP
#define AGAME_HPP

#include "GeneralEntity.hpp"
#include "EngineFrame.hpp"
#include <map>

class AGame {
    public:
        AGame() = default;
        virtual ~AGame() = default;
        virtual void run(int numPlayers) = 0;
        virtual void addPlayerAction(int playerId, int action) = 0;
        virtual std::map<int, GeneralEntity>& getEntities() = 0;
        virtual std::pair<float, float> getEntityPosition(int entityId) const = 0;
        virtual std::map<int, EngineFrame>& getEngineFrames() = 0;
};

#endif // AGAME_HPP
