#pragma once

class Game;

class AdminPanel {
public:
    explicit AdminPanel(Game& game);
    void openConsole();

private:
    void printMenu() const;
    bool validateChances() const;
    Game& m_game;
};
