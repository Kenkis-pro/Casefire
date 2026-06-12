#include "Game.h"

#include "AdminPanel.h"
#include "Shop.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

using json = nlohmann::json;

namespace {
constexpr float kPlayerSpeed = 5.0f;
constexpr float kMouseSensitivity = 0.08f;
constexpr float kEnemySpeed = 1.2f;
constexpr float kShootRange = 22.0f;
constexpr float kPi = 3.14159265359f;

float distancePointToRay(const glm::vec3& point, const glm::vec3& rayOrigin, const glm::vec3& rayDir, float& alongRay) {
    alongRay = glm::dot(point - rayOrigin, rayDir);
    glm::vec3 closest = rayOrigin + rayDir * alongRay;
    return glm::length(point - closest);
}

ALuint makeTone(float frequency, float durationSeconds) {
    constexpr int sampleRate = 22050;
    const int sampleCount = static_cast<int>(sampleRate * durationSeconds);
    std::vector<short> samples(sampleCount);
    for (int i = 0; i < sampleCount; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(sampleRate);
        float envelope = 1.0f - static_cast<float>(i) / static_cast<float>(sampleCount);
        samples[i] = static_cast<short>(std::sin(2.0f * kPi * frequency * t) * 15000.0f * envelope);
    }

    ALuint buffer = 0;
    alGenBuffers(1, &buffer);
    alBufferData(buffer, AL_FORMAT_MONO16, samples.data(), static_cast<ALsizei>(samples.size() * sizeof(short)), sampleRate);
    return buffer;
}
}

Game::Game() = default;

Game::~Game() {
    saveAll();
    if (m_cubeVbo) glDeleteBuffers(1, &m_cubeVbo);
    if (m_cubeVao) glDeleteVertexArrays(1, &m_cubeVao);
    if (m_shader) glDeleteProgram(m_shader);
    shutdownAudio();
    if (m_window) glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool Game::init() {
    ensureDefaultJsonFiles();
    loadAll();
    if (!initWindowAndGL()) return false;
    if (!initShaders()) return false;
    if (!initGeometry()) return false;
    initAudio();
    spawnEnemies();
    updateTitle();
    return true;
}

void Game::run() {
    float lastTime = static_cast<float>(glfwGetTime());
    while (!glfwWindowShouldClose(m_window)) {
        float now = static_cast<float>(glfwGetTime());
        float dt = now - lastTime;
        lastTime = now;

        processInput(dt);
        update(dt);
        render();
        updateTitle();

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }
}

bool Game::initWindowAndGL() {
    if (!glfwInit()) return false;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(m_width, m_height, "CASEFIRE", nullptr, nullptr);
    if (!m_window) return false;
    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetCursorPosCallback(m_window, mouseCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetKeyCallback(m_window, keyCallback);
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

#if defined(CASEFIRE_GLAD_LEGACY)
    bool gladOk = gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));
#elif defined(CASEFIRE_GLAD_GL)
    bool gladOk = gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)) != 0;
#endif
    if (!gladOk) {
        std::cerr << "Failed to initialize glad.\n";
        return false;
    }
    glViewport(0, 0, m_width, m_height);
    glEnable(GL_DEPTH_TEST);
    return true;
}

bool Game::initAudio() {
    m_audioDevice = alcOpenDevice(nullptr);
    if (!m_audioDevice) return false;
    m_audioContext = alcCreateContext(m_audioDevice, nullptr);
    if (!m_audioContext || !alcMakeContextCurrent(m_audioContext)) return false;

    m_buffers[SoundId::Shoot] = makeTone(880.0f, 0.08f);
    m_buffers[SoundId::Hit] = makeTone(520.0f, 0.10f);
    m_buffers[SoundId::CaseOpen] = makeTone(1320.0f, 0.18f);
    m_buffers[SoundId::Error] = makeTone(150.0f, 0.20f);
    for (auto& [id, buffer] : m_buffers) {
        ALuint source = 0;
        alGenSources(1, &source);
        alSourcei(source, AL_BUFFER, static_cast<ALint>(buffer));
        m_sources[id] = source;
    }
    return true;
}

void Game::shutdownAudio() {
    for (auto& [id, source] : m_sources) alDeleteSources(1, &source);
    for (auto& [id, buffer] : m_buffers) alDeleteBuffers(1, &buffer);
    if (m_audioContext) {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(m_audioContext);
    }
    if (m_audioDevice) alcCloseDevice(m_audioDevice);
}

bool Game::initGeometry() {
    float vertices[] = {
        -0.5f,-0.5f,-0.5f, 0.5f,-0.5f,-0.5f, 0.5f, 0.5f,-0.5f, 0.5f, 0.5f,-0.5f,-0.5f, 0.5f,-0.5f,-0.5f,-0.5f,-0.5f,-0.5f,
        -0.5f,-0.5f, 0.5f, 0.5f,-0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,-0.5f, 0.5f, 0.5f,-0.5f,-0.5f, 0.5f,
        -0.5f, 0.5f, 0.5f,-0.5f, 0.5f,-0.5f,-0.5f,-0.5f,-0.5f,-0.5f,-0.5f,-0.5f,-0.5f,-0.5f, 0.5f,-0.5f, 0.5f, 0.5f,
         0.5f, 0.5f, 0.5f, 0.5f, 0.5f,-0.5f, 0.5f,-0.5f,-0.5f, 0.5f,-0.5f,-0.5f, 0.5f,-0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
        -0.5f,-0.5f,-0.5f, 0.5f,-0.5f,-0.5f, 0.5f,-0.5f, 0.5f, 0.5f,-0.5f, 0.5f,-0.5f,-0.5f, 0.5f,-0.5f,-0.5f,-0.5f,
        -0.5f, 0.5f,-0.5f, 0.5f, 0.5f,-0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,-0.5f, 0.5f, 0.5f,-0.5f, 0.5f,-0.5f
    };
    glGenVertexArrays(1, &m_cubeVao);
    glGenBuffers(1, &m_cubeVbo);
    glBindVertexArray(m_cubeVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_cubeVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    return true;
}

bool Game::initShaders() {
    GLuint vertex = compileShader(GL_VERTEX_SHADER, readFile("shaders/basic.vert"));
    GLuint fragment = compileShader(GL_FRAGMENT_SHADER, readFile("shaders/basic.frag"));
    if (!vertex || !fragment) return false;
    m_shader = glCreateProgram();
    glAttachShader(m_shader, vertex);
    glAttachShader(m_shader, fragment);
    glLinkProgram(m_shader);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    int success = 0;
    glGetProgramiv(m_shader, GL_LINK_STATUS, &success);
    return success == GL_TRUE;
}

void Game::processInput(float dt) {
    if (m_state != ScreenState::Playing) return;
    glm::vec3 right = glm::normalize(glm::cross(m_front, m_up));
    glm::vec3 planarFront = glm::normalize(glm::vec3(m_front.x, 0.0f, m_front.z));
    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) m_playerPos += planarFront * kPlayerSpeed * dt;
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) m_playerPos -= planarFront * kPlayerSpeed * dt;
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) m_playerPos -= right * kPlayerSpeed * dt;
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) m_playerPos += right * kPlayerSpeed * dt;
    m_playerPos.y = 1.0f;
}

void Game::update(float dt) {
    if (m_shootCooldown > 0.0f) m_shootCooldown -= dt;
    if (m_profile.banned) {
        m_state = ScreenState::Banned;
        return;
    }
    if (m_state != ScreenState::Playing) return;

    for (auto& enemy : m_enemies) {
        glm::vec3 toPlayer = m_playerPos - enemy.position;
        toPlayer.y = 0.0f;
        float distance = glm::length(toPlayer);
        if (distance > 0.1f) enemy.position += glm::normalize(toPlayer) * kEnemySpeed * dt;
        enemy.attackCooldown -= dt;
        if (distance < 1.1f && enemy.attackCooldown <= 0.0f) {
            m_profile.health -= 10;
            enemy.attackCooldown = 1.0f;
            playSound(SoundId::Hit);
            if (m_profile.health <= 0) {
                m_profile.health = 0;
                m_state = ScreenState::GameOver;
                setMessage("GAME OVER - press R to restart");
                saveAll();
            }
        }
    }
}

void Game::render() {
    glm::vec3 clear = m_state == ScreenState::GameOver ? glm::vec3(0.35f, 0.02f, 0.02f) : glm::vec3(0.04f, 0.05f, 0.08f);
    glClearColor(clear.r, clear.g, clear.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 projection = glm::perspective(glm::radians(70.0f), static_cast<float>(m_width) / static_cast<float>(m_height), 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(m_playerPos, m_playerPos + m_front, m_up);
    renderScene3D(view, projection);
    renderCrosshairAndGun();
}

void Game::renderScene3D(const glm::mat4& view, const glm::mat4& projection) {
    glUseProgram(m_shader);
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(glGetUniformLocation(m_shader, "lightDir"), -0.3f, -1.0f, -0.4f);
    glUniform1f(glGetUniformLocation(m_shader, "ambientStrength"), 0.35f);

    renderCube({0.0f, -0.05f, 0.0f}, {30.0f, 0.1f, 30.0f}, {0.18f, 0.18f, 0.18f});
    for (const auto& wall : m_walls) renderCube(wall, {2.0f, 2.0f, 2.0f}, {0.35f, 0.37f, 0.42f});
    for (const auto& enemy : m_enemies) renderCube(enemy.position, {0.9f, 0.9f, 0.9f}, {0.85f, 0.08f, 0.06f});
}

void Game::renderCube(const glm::vec3& position, const glm::vec3& scale, const glm::vec3& color) {
    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, scale);
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(m_shader, "objectColor"), 1, glm::value_ptr(color));
    glBindVertexArray(m_cubeVao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void Game::renderCrosshairAndGun() {
    const Skin* skin = findSkin(m_profile.equippedSkin);
    glm::vec3 gunColor = skin ? skin->color : glm::vec3(0.25f);
    glm::mat4 view(1.0f);
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(m_width), 0.0f, static_cast<float>(m_height), -1.0f, 1.0f);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(m_shader);
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    renderCube({m_width / 2.0f, m_height / 2.0f, 0.0f}, {4.0f, 24.0f, 1.0f}, {1.0f, 1.0f, 1.0f});
    renderCube({m_width / 2.0f, m_height / 2.0f, 0.0f}, {24.0f, 4.0f, 1.0f}, {1.0f, 1.0f, 1.0f});
    renderCube({m_width * 0.63f, 80.0f, 0.0f}, {150.0f, 70.0f, 1.0f}, gunColor);
    glEnable(GL_DEPTH_TEST);
}

void Game::shoot() {
    if (m_state != ScreenState::Playing || m_shootCooldown > 0.0f) return;
    m_shootCooldown = 0.25f;
    playSound(SoundId::Shoot);

    int hitIndex = -1;
    float bestDistance = kShootRange;
    for (int i = 0; i < static_cast<int>(m_enemies.size()); ++i) {
        float along = 0.0f;
        float distance = distancePointToRay(m_enemies[i].position, m_playerPos, glm::normalize(m_front), along);
        if (along > 0.0f && along < bestDistance && distance < 0.8f) {
            bestDistance = along;
            hitIndex = i;
        }
    }

    if (hitIndex >= 0) {
        playSound(SoundId::Hit);
        Enemy& enemy = m_enemies[hitIndex];
        enemy.hp -= 1;
        if (enemy.hp <= 0) {
            m_profile.coins += 10;
            m_enemies.erase(m_enemies.begin() + hitIndex);
            if (m_enemies.empty()) spawnEnemies();
            saveAll();
        }
    }
}

void Game::spawnEnemies() {
    m_enemies.clear();
    m_enemies.push_back({{-5.0f, 0.5f, -4.0f}, 3, 0.0f});
    m_enemies.push_back({{4.0f, 0.5f, -7.0f}, 3, 0.0f});
    m_enemies.push_back({{7.0f, 0.5f, 2.0f}, 3, 0.0f});
    m_walls = {{-2.0f, 1.0f, -2.0f}, {3.0f, 1.0f, -1.0f}, {0.0f, 1.0f, -6.0f}, {-6.0f, 1.0f, 3.0f}};
}

void Game::playSound(SoundId id) {
    auto it = m_sources.find(id);
    if (it == m_sources.end()) return;
    alSourceStop(it->second);
    alSourcePlay(it->second);
}

void Game::setMessage(const std::string& message) {
    m_statusMessage = message;
}

bool Game::hasSkin(int id) const {
    return std::find(m_inventory.begin(), m_inventory.end(), id) != m_inventory.end();
}

const Skin* Game::findSkin(int id) const {
    for (const auto& skin : m_skins) {
        if (skin.id == id) return &skin;
    }
    return nullptr;
}

void Game::equipSkin(int id) {
    if (id == 0 || hasSkin(id)) {
        m_profile.equippedSkin = id;
        const Skin* skin = findSkin(id);
        setMessage(std::string("Equipped ") + (skin ? skin->name : "default"));
    }
}

void Game::restartMatch() {
    m_profile.health = 100;
    m_playerPos = {0.0f, 1.0f, 6.0f};
    m_state = ScreenState::Playing;
    spawnEnemies();
    saveAll();
}

void Game::reloadAdminFlag() {
    std::ifstream in("admin.json");
    json data = json::parse(in, nullptr, false);
    m_adminEnabled = !data.is_discarded() && data.value("admin", false);
}

void Game::updateTitle() {
    std::ostringstream title;
    if (m_state == ScreenState::MainMenu) {
        title << "CASEFIRE | MAIN MENU | ENTER play | S shop | F1 admin | " << m_statusMessage;
    } else if (m_state == ScreenState::Shop) {
        Shop shop(*this);
        title << shop.statusLine();
    } else if (m_state == ScreenState::GameOver) {
        title << "CASEFIRE | GAME OVER | R restart | Coins: " << m_profile.coins;
    } else if (m_state == ScreenState::Banned) {
        title << "CASEFIRE | PLAYER BANNED | Ask admin to unban in profile.json/admin panel";
    } else {
        title << "CASEFIRE | HP: " << m_profile.health << " | Coins: " << m_profile.coins
              << " | Enemies: " << m_enemies.size() << " | F1 admin | " << m_statusMessage;
    }
    glfwSetWindowTitle(m_window, title.str().c_str());
}

void Game::saveAll() {
    json profile = {
        {"coins", m_profile.coins}, {"health", m_profile.health},
        {"banned", m_profile.banned}, {"equippedSkin", m_profile.equippedSkin}
    };
    std::ofstream("profile.json") << profile.dump(4);
    std::ofstream("inventory.json") << json{{"skins", m_inventory}}.dump(4);

    json skins = json::array();
    for (const auto& skin : m_skins) {
        skins.push_back({{"id", skin.id}, {"name", skin.name}, {"color", {skin.color.r, skin.color.g, skin.color.b}}, {"price", skin.price}, {"chance", skin.chance}});
    }
    std::ofstream("skins.json") << skins.dump(4);
    std::ofstream("case_config.json") << json{{"caseCost", m_caseCost}}.dump(4);
}

void Game::loadAll() {
    auto parseFile = [](const std::string& path) {
        std::ifstream in(path);
        return json::parse(in, nullptr, false);
    };

    json profile = parseFile("profile.json");
    if (!profile.is_discarded()) {
        m_profile.coins = profile.value("coins", 0);
        m_profile.health = profile.value("health", 100);
        m_profile.banned = profile.value("banned", false);
        m_profile.equippedSkin = profile.value("equippedSkin", 0);
    }

    json inventory = parseFile("inventory.json");
    m_inventory = inventory.value("skins", std::vector<int>{0});
    if (!hasSkin(0)) m_inventory.insert(m_inventory.begin(), 0);

    json skinsData = parseFile("skins.json");
    m_skins.clear();
    if (skinsData.is_array()) {
        for (const auto& item : skinsData) {
            auto color = item.value("color", std::vector<float>{0.25f, 0.25f, 0.25f});
            if (color.size() < 3) color = {0.25f, 0.25f, 0.25f};
            m_skins.push_back({item.value("id", 0), item.value("name", std::string("Skin")), {color[0], color[1], color[2]}, item.value("price", 100), item.value("chance", 20.0f)});
        }
    }

    json caseConfig = parseFile("case_config.json");
    if (!caseConfig.is_discarded()) m_caseCost = caseConfig.value("caseCost", 100);
    reloadAdminFlag();
}

void Game::ensureDefaultJsonFiles() {
    if (!std::filesystem::exists("profile.json")) {
        std::ofstream("profile.json") << json{{"coins", 150}, {"health", 100}, {"banned", false}, {"equippedSkin", 0}}.dump(4);
    }
    if (!std::filesystem::exists("inventory.json")) {
        std::ofstream("inventory.json") << json{{"skins", std::vector<int>{0}}}.dump(4);
    }
    if (!std::filesystem::exists("admin.json")) {
        std::ofstream("admin.json") << json{{"admin", true}}.dump(4);
    }
    if (!std::filesystem::exists("case_config.json")) {
        std::ofstream("case_config.json") << json{{"caseCost", 100}}.dump(4);
    }
    if (!std::filesystem::exists("skins.json")) {
        json skins = json::array({
            {{"id", 0}, {"name", "Standard"}, {"color", {0.24f, 0.24f, 0.24f}}, {"price", 0}, {"chance", 45.0f}},
            {{"id", 1}, {"name", "Blue"}, {"color", {0.1f, 0.25f, 0.95f}}, {"price", 100}, {"chance", 25.0f}},
            {{"id", 2}, {"name", "Red"}, {"color", {0.9f, 0.08f, 0.05f}}, {"price", 150}, {"chance", 15.0f}},
            {{"id", 3}, {"name", "Gold"}, {"color", {1.0f, 0.76f, 0.05f}}, {"price", 300}, {"chance", 10.0f}},
            {{"id", 4}, {"name", "Rainbow"}, {"color", {0.75f, 0.15f, 1.0f}}, {"price", 500}, {"chance", 5.0f}}
        });
        std::ofstream("skins.json") << skins.dump(4);
    }
}

std::string Game::readFile(const std::string& path) const {
    std::ifstream file(path);
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint Game::compileShader(GLenum type, const std::string& source) const {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "Shader compile error: " << log << "\n";
        return 0;
    }
    return shader;
}

void Game::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (!game || game->m_state != ScreenState::Playing) return;
    if (game->m_firstMouse) {
        game->m_lastMouseX = static_cast<float>(xpos);
        game->m_lastMouseY = static_cast<float>(ypos);
        game->m_firstMouse = false;
    }
    float xoffset = (static_cast<float>(xpos) - game->m_lastMouseX) * kMouseSensitivity;
    float yoffset = (game->m_lastMouseY - static_cast<float>(ypos)) * kMouseSensitivity;
    game->m_lastMouseX = static_cast<float>(xpos);
    game->m_lastMouseY = static_cast<float>(ypos);

    game->m_yaw += xoffset;
    game->m_pitch = glm::clamp(game->m_pitch + yoffset, -89.0f, 89.0f);
    glm::vec3 front;
    front.x = std::cos(glm::radians(game->m_yaw)) * std::cos(glm::radians(game->m_pitch));
    front.y = std::sin(glm::radians(game->m_pitch));
    front.z = std::sin(glm::radians(game->m_yaw)) * std::cos(glm::radians(game->m_pitch));
    game->m_front = glm::normalize(front);
}

void Game::mouseButtonCallback(GLFWwindow* window, int button, int action, int) {
    auto* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) game->shoot();
}

void Game::keyCallback(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    auto* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (!game) return;

    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);
    if (key == GLFW_KEY_F1) {
        AdminPanel panel(*game);
        panel.openConsole();
    }

    if (game->m_state == ScreenState::MainMenu) {
        if (key == GLFW_KEY_ENTER) game->restartMatch();
        if (key == GLFW_KEY_S) game->m_state = ScreenState::Shop;
    } else if (game->m_state == ScreenState::Shop) {
        if (key == GLFW_KEY_B) game->m_state = ScreenState::MainMenu;
        Shop shop(*game);
        shop.handleKey(key);
    } else if (game->m_state == ScreenState::GameOver && key == GLFW_KEY_R) {
        game->restartMatch();
    } else if (game->m_state == ScreenState::Banned && key == GLFW_KEY_F1) {
        game->loadAll();
        if (!game->m_profile.banned) game->m_state = ScreenState::MainMenu;
    }
}
