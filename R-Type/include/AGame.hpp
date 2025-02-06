/*
** EPITECH PROJECT, 2025
** R-Type [WSL: Ubuntu]
** File description:
** AGame
*/

#ifndef AGAME_HPP
#define AGAME_HPP

class AGame {
    public:
        AGame() = default;
        virtual ~AGame() = default;
        virtual void run(int numPlayers) = 0;
};

#endif // AGAME_HPP
