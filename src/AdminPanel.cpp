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
        m_game.setMessage("Admin panel denied: admin.json has admin=false");
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
            std::cout << "Coins to add: ";
            std::cin >> amount;
            m_game.profile().coins = std::max(0, m_game.profile().coins + amount);
            m_game.saveAll();
            break;
        }
        case 2: {
            int id = 0;
            std::cout << "Skin id to grant: ";
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
            std::cout << "New case cost: ";
            std::cin >> cost;
            m_game.caseCost() = std::max(0, cost);
            m_game.saveAll();
            break;
        }
        case 4: {
            std::cout << "Enter chances in percent. Sum must be 100.\n";
            for (auto& skin : m_game.skins()) {
                std::cout << skin.id << " " << skin.name << ": ";
                std::cin >> skin.chance;
            }
            if (!validateChances()) {
                std::cout << "Warning: chances do not sum to 100; normalize them manually for predictable drops.\n";
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
    m_game.setMessage("Admin changes saved and applied");
}

void AdminPanel::printMenu() const {
    std::cout << "\n=== CASEFIRE ADMIN ===\n"
              << "1. Add coins\n"
              << "2. Grant skin by id\n"
              << "3. Change case cost\n"
              << "4. Change skin drop chances\n"
              << "5. Ban player\n"
              << "6. Unban player\n"
              << "7. Exit panel\n"
              << "Choice: ";
}

bool AdminPanel::validateChances() const {
    float sum = 0.0f;
    for (const auto& skin : m_game.skins()) {
        sum += skin.chance;
    }
    return sum > 99.9f && sum < 100.1f;
}
