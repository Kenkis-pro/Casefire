#include "Shop.h"

#include "Game.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <numeric>
#include <random>
#include <sstream>

Shop::Shop(Game& game) : m_game(game) {}

void Shop::handleKey(int key) {
    if (key == GLFW_KEY_O) {
        openCase();
    } else if (key == GLFW_KEY_E) {
        equipNextSkin();
    } else if (key == GLFW_KEY_X) {
        sellEquippedSkin();
    }
}

void Shop::openCase() {
    if (m_game.profile().coins < m_game.caseCost()) {
        m_game.playSound(SoundId::Error);
        m_game.setMessage("Недостаточно монет для открытия кейса");
        return;
    }

    const int skinId = rollSkinId();
    const Skin* skin = m_game.findSkin(skinId);
    m_game.profile().coins -= m_game.caseCost();
    if (!m_game.hasSkin(skinId)) {
        m_game.inventory().push_back(skinId);
    }
    m_game.equipSkin(skinId);
    m_game.saveAll();
    m_game.playSound(SoundId::CaseOpen);
    m_game.setMessage(std::string("Кейс открыт: ") + (skin ? skin->name : "неизвестный скин"));
}

void Shop::sellEquippedSkin() {
    const int id = m_game.profile().equippedSkin;
    if (id == 0) {
        m_game.playSound(SoundId::Error);
        m_game.setMessage("Стандартный скин нельзя продать");
        return;
    }

    auto& inventory = m_game.inventory();
    auto it = std::find(inventory.begin(), inventory.end(), id);
    const Skin* skin = m_game.findSkin(id);
    if (it == inventory.end() || !skin) {
        m_game.playSound(SoundId::Error);
        m_game.setMessage("Выбранного скина нет в инвентаре");
        return;
    }

    m_game.profile().coins += skin->price / 2;
    inventory.erase(it);
    m_game.equipSkin(0);
    m_game.saveAll();
    m_game.setMessage("Скин продан за 50% цены");
}

void Shop::equipNextSkin() {
    if (m_game.inventory().empty()) {
        m_game.equipSkin(0);
        return;
    }

    auto& inventory = m_game.inventory();
    auto it = std::find(inventory.begin(), inventory.end(), m_game.profile().equippedSkin);
    if (it == inventory.end() || ++it == inventory.end()) {
        it = inventory.begin();
    }
    m_game.equipSkin(*it);
    m_game.saveAll();
}

std::string Shop::statusLine() const {
    const Skin* equipped = m_game.findSkin(m_game.profile().equippedSkin);
    std::ostringstream out;
    out << "МАГАЗИН | Монеты: " << m_game.profile().coins
        << " | Кейс: " << m_game.caseCost()
        << " | Выбрано: " << (equipped ? equipped->name : "неизвестно")
        << " | O открыть, E следующий скин, X продать, B назад";
    return out.str();
}

int Shop::rollSkinId() const {
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> dist(0.0f, 100.0f);
    float roll = dist(rng);
    float cumulative = 0.0f;
    for (const auto& skin : m_game.skins()) {
        cumulative += skin.chance;
        if (roll <= cumulative) {
            return skin.id;
        }
    }
    return m_game.skins().empty() ? 0 : m_game.skins().back().id;
}
