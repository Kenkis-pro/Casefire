#include "AdminPanel.h"

#include "Game.h"

#include <algorithm>
#include <iostream>
#include <limits>

AdminPanel::AdminPanel(Game& game) : m_game(game) {}

void AdminPanel::openConsole() {
    m_game.reloadAdminFlag();
    if (!m_game.isAdmin()) {
        m_game.playSound(SoundId::Error);
        m_game.setMessage("Доступ запрещён: в admin.json указано admin=false");
        return;
    }

    bool done = false;
    while (!done) {
        printMenu();
        int choice = 0;
        std::cin >> choice;
        if (!std::cin) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        switch (choice) {
        case 1: {
            int amount = 0;
            std::cout << "Сколько монет начислить: ";
            std::cin >> amount;
            m_game.profile().coins = std::max(0, m_game.profile().coins + amount);
            m_game.saveAll();
            break;
        }
        case 2: {
            int id = 0;
            std::cout << "ID скина для выдачи: ";
            std::cin >> id;
            if (m_game.findSkin(id) && !m_game.hasSkin(id)) {
                m_game.inventory().push_back(id);
            }
            if (m_game.findSkin(id)) {
                m_game.equipSkin(id);
            }
            m_game.saveAll();
            break;
        }
        case 3: {
            int cost = 100;
            std::cout << "Новая цена кейса: ";
            std::cin >> cost;
            m_game.caseCost() = std::max(0, cost);
            m_game.saveAll();
            break;
        }
        case 4: {
            std::cout << "Введите шансы в процентах. Сумма должна быть 100.\n";
            for (auto& skin : m_game.skins()) {
                std::cout << skin.id << " " << skin.name << ": ";
                std::cin >> skin.chance;
            }
            if (!validateChances()) {
                std::cout << "Предупреждение: сумма шансов не равна 100; исправьте значения для предсказуемых выпадений.\n";
            }
            m_game.saveAll();
            break;
        }
        case 5: {
            m_game.profile().banned = true;
            m_game.saveAll();
            break;
        }
        case 6: {
            m_game.profile().banned = false;
            m_game.saveAll();
            break;
        }
        case 7:
            done = true;
            break;
        default:
            break;
        }
    }
    m_game.setMessage("Изменения админ-панели сохранены и применены");
}

void AdminPanel::printMenu() const {
    std::cout << "\n=== АДМИН-ПАНЕЛЬ CASEFIRE ===\n"
              << "1. Начислить монеты\n"
              << "2. Выдать скин по ID\n"
              << "3. Изменить цену кейса\n"
              << "4. Изменить шансы выпадения скинов\n"
              << "5. Забанить игрока\n"
              << "6. Разбанить игрока\n"
              << "7. Выйти из панели\n"
              << "Выбор: ";
}

bool AdminPanel::validateChances() const {
    float sum = 0.0f;
    for (const auto& skin : m_game.skins()) {
        sum += skin.chance;
    }
    return sum > 99.9f && sum < 100.1f;
}
