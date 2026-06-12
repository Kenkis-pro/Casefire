#pragma once

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <AL/al.h>
#include <AL/alc.h>

#include <map>
#include <random>
#include <string>
#include <vector>

struct Skin {
    int id{};
    std::string name;
    glm::vec3 color{0.25f, 0.25f, 0.25f};
    int price{};
    float chance{};
};

struct Profile {
    int coins{0};
    int health{100};
    bool banned{false};
    int equippedSkin{0};
};

struct Enemy {
    glm::vec3 position{0.0f};
    int hp{3};
    float attackCooldown{0.0f};
};

enum class ScreenState {
    MainMenu,
    Playing,
    Shop,
    GameOver,
    Banned
};

enum class SoundId {
    Shoot = 0,
    Hit,
    CaseOpen,
    Error
};

class Game {
public:
    Game();
    ~Game();

    bool init();
    void run();

    Profile& profile() { return m_profile; }
    std::vector<int>& inventory() { return m_inventory; }
    std::vector<Skin>& skins() { return m_skins; }
    int& caseCost() { return m_caseCost; }
    GLFWwindow* window() { return m_window; }

    void saveAll();
    void loadAll();
    void playSound(SoundId id);
    void setMessage(const std::string& message);
    bool hasSkin(int id) const;
    const Skin* findSkin(int id) const;
    void equipSkin(int id);
    void restartMatch();
    void reloadAdminFlag();
    bool isAdmin() const { return m_adminEnabled; }

private:
    friend class Shop;
    friend class AdminPanel;

    bool initWindowAndGL();
    bool initAudio();
    bool initGeometry();
    bool initShaders();
    void shutdownAudio();

    void processInput(float dt);
    void update(float dt);
    void render();
    void renderScene3D(const glm::mat4& view, const glm::mat4& projection);
    void renderCube(const glm::vec3& position, const glm::vec3& scale, const glm::vec3& color);
    void renderCrosshairAndGun();
    void shoot();
    void spawnEnemies();
    void updateTitle();
    void ensureDefaultJsonFiles();

    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    std::string readFile(const std::string& path) const;
    GLuint compileShader(GLenum type, const std::string& source) const;

    GLFWwindow* m_window{nullptr};
    int m_width{1280};
    int m_height{720};
    ScreenState m_state{ScreenState::MainMenu};

    GLuint m_shader{0};
    GLuint m_cubeVao{0};
    GLuint m_cubeVbo{0};

    glm::vec3 m_playerPos{0.0f, 1.0f, 6.0f};
    float m_yaw{-90.0f};
    float m_pitch{0.0f};
    glm::vec3 m_front{0.0f, 0.0f, -1.0f};
    glm::vec3 m_up{0.0f, 1.0f, 0.0f};
    bool m_firstMouse{true};
    float m_lastMouseX{640.0f};
    float m_lastMouseY{360.0f};
    float m_shootCooldown{0.0f};

    Profile m_profile;
    std::vector<int> m_inventory;
    std::vector<Skin> m_skins;
    int m_caseCost{100};
    bool m_adminEnabled{false};
    std::string m_statusMessage{"ENTER — играть, S — магазин, ESC — выход"};

    std::vector<Enemy> m_enemies;
    std::vector<glm::vec3> m_walls;

    ALCdevice* m_audioDevice{nullptr};
    ALCcontext* m_audioContext{nullptr};
    std::map<SoundId, ALuint> m_buffers;
    std::map<SoundId, ALuint> m_sources;
};
