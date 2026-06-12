#pragma once

#include <string>

class Game;

class Shop {
public:
    explicit Shop(Game& game);

    void handleKey(int key);
    void openCase();
    void sellEquippedSkin();
    void equipNextSkin();
    std::string statusLine() const;

private:
    int rollSkinId() const;
    Game& m_game;
};
