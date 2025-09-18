#include "Game.h"
#include "Graphics.h"
#include "Player.h"
#include "EnemyManager.h"
#include "ProjectileManager.h"
#include "MountainManager.h"
#include "UserInterface.h"
#include "BoatSkinIds.h"
#include <glm/glm.hpp>
#include <iostream>

// callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

Game::Game(unsigned int width, unsigned int height)
    : window(nullptr), screenWidth(width), screenHeight(height),
      state(MAIN_MENU), difficulty(EASY), gameTime(0.0f), score(0),
      enemiesDestroyed(0), finalScore(0), waveTime(0.0f),
      cameraYaw(0.0f), cameraPitch(0.0f), cameraDistance(15.0f), cameraHeight(8.0f),
      firstMouse(true), lastX(width / 2.0), lastY(height / 2.0), isFirstPerson(false),
      enableRainbowWater(false), enableCrazyPhysics(false), enablePartyMode(false),
      boatSkinIndex(0),
      selectedMenuItem(0), selectedDifficultyItem(0), selectedSettingsItem(0), selectedPauseItem(0) {
    shipCamera.mode = Camera::THIRD_PERSON;
    shipCamera.SetYaw(90.0f);
    shipCamera.SetPitch(-15.0f);
}

Game::~Game() {
    glfwTerminate();
}

void Game::SetState(GameState s) {
    if (state == s) return;
    state = s;
    if (!window) return;
    if (state == PLAYING) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        firstMouse = true;
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}


void Game::Init() {
    // glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(screenWidth, screenHeight, "Boat Escape", nullptr, nullptr);
    if (!window) {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, this);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to init GLAD\n";
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    graphics          = std::make_unique<Graphics>();
    player            = std::make_unique<Player>();
    enemyManager      = std::make_unique<EnemyManager>();
    projectileManager = std::make_unique<ProjectileManager>();
    mountainManager   = std::make_unique<MountainManager>();
    ui                = std::make_unique<UserInterface>();

    graphics->Init();
    ui->Init();

    std::cout << "Game initialized.\n";
}

void Game::ProcessInput(float dt) {
    if (state == PLAYING) {
        static bool fpTogglePrev = false;
        static bool tpTogglePrev = false;
        bool key1Down = glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS;
        bool key2Down = glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS;

        if (boatSkinIndex == BoatSkinId::GOING_MERRY) {
            if (key1Down && !fpTogglePrev) {
                shipCamera.mode = Camera::FIRST_PERSON;
                shipCamera.SetPitch(0.0f);
            }
            if (key2Down && !tpTogglePrev) {
                shipCamera.mode = Camera::THIRD_PERSON;
                shipCamera.SetYaw(player->GetRotation());
            }
            isFirstPerson = (shipCamera.mode == Camera::FIRST_PERSON);
        } else {
            if (key1Down) {
                isFirstPerson = true;
                if (boatSkinIndex == BoatSkinId::BIG_MOM) cameraPitch = 0.0f;
            }
            if (key2Down) {
                isFirstPerson = false;
                if (boatSkinIndex == BoatSkinId::BIG_MOM) cameraYaw = 90.0f - player->GetRotation();
            }
        }

        fpTogglePrev = key1Down;
        tpTogglePrev = key2Down;

        static bool cPrev = false;
        bool cNow = glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS;
        if (cNow && !cPrev) enableDebugMountains = !enableDebugMountains;
        cPrev = cNow;

        player->SetPhysicsMode(enableCrazyPhysics);
        player->SetBoatSkin(boatSkinIndex);
        player->ProcessGameInput(window, dt, *projectileManager, *mountainManager);

        if (boatSkinIndex == BoatSkinId::GOING_MERRY) {
            player->AlignToWater(-1.0f);
        }

        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
            SetState(PAUSED);
        }
    } else {
        ProcessMenuInput(dt);
    }
}

void Game::Update(float dt) {
    if (state == PLAYING) {
        gameTime += dt;
        waveTime += dt;

        player->Update(dt);
        projectileManager->Update(dt, *mountainManager);
        enemyManager->Update(dt, player->GetPosition(), *projectileManager, *mountainManager);
        mountainManager->Update(dt, player->GetPosition());

        checkCollisions();

        if (player->GetHealth() <= 0) {
            triggerGameOver();
        }
    }
}

void Game::Render() {
    glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (state == PLAYING || state == PAUSED) {
        graphics->Render(*player, *enemyManager, *projectileManager, *mountainManager,
                         waveTime, cameraYaw, cameraPitch, cameraDistance, cameraHeight,
                         isFirstPerson,
                         enableRainbowWater,
                         enablePartyMode,
                         boatSkinIndex,
                         enableDebugMountains,
                         shipCamera,
                         player->GetShipFront());

        ui->RenderHUD(player->GetHealth(), player->GetMaxHealth(), score, gameTime, enemiesDestroyed, difficulty);
        if (state == PAUSED) ui->RenderPauseScreen(selectedPauseItem);
    } else if (state == GAME_OVER) {
        ui->RenderGameOverScreen(finalScore, enemiesDestroyed, gameTime, difficulty);
    } else {
        ui->RenderMenu(state, selectedMenuItem, selectedDifficultyItem, selectedSettingsItem,
                       enableRainbowWater, enableCrazyPhysics, enablePartyMode, boatSkinIndex);
    }
}

void Game::startNewGame() {
    SetState(PLAYING);
    player->Reset();
    enemyManager->Init(difficulty);
    projectileManager->Clear();
    mountainManager->Init();
    gameTime = 0.0f;
    score = 0;
    enemiesDestroyed = 0;
    waveTime = 0.0f;
    finalScore = 0;
    firstMouse = true;
    isFirstPerson = false;
    shipCamera.mode = Camera::THIRD_PERSON;
    shipCamera.SetYaw(90.0f);
    shipCamera.SetPitch(-15.0f);
    cameraYaw = 0.0f;
    cameraPitch = 0.0f;
}

void Game::triggerGameOver() {
    SetState(GAME_OVER);
    finalScore = score;
}

void Game::checkCollisions() {
    auto& projs = projectileManager->GetProjectiles();
    auto& enemies = enemyManager->GetEnemies();

    for (auto it = projs.begin(); it != projs.end();) {
        bool removed = false;
        if (it->isPlayerOwned) {
            for (auto& e : enemies) {
                if (e.active && glm::length(it->position - e.position) < 2.0f) {
                    e.active = false;
                    score += 100;
                    enemiesDestroyed++;
                    it = projs.erase(it);
                    removed = true;
                    break;
                }
            }
        } else {
            if (glm::length(it->position - player->GetPosition()) < 1.5f) {
                player->TakeDamage(20);
                it = projs.erase(it);
                removed = true;
            }
        }
        if (!removed) ++it;
    }
}

void Game::ProcessMenuInput(float) {
    static bool upPressed=false, downPressed=false, leftPressed=false, rightPressed=false, enterPressed=false, escPressed=false;

    bool up    = glfwGetKey(window, GLFW_KEY_UP)==GLFW_PRESS   || glfwGetKey(window, GLFW_KEY_W)==GLFW_PRESS;
    bool down  = glfwGetKey(window, GLFW_KEY_DOWN)==GLFW_PRESS || glfwGetKey(window, GLFW_KEY_S)==GLFW_PRESS;
    bool left  = glfwGetKey(window, GLFW_KEY_LEFT)==GLFW_PRESS || glfwGetKey(window, GLFW_KEY_A)==GLFW_PRESS;
    bool right = glfwGetKey(window, GLFW_KEY_RIGHT)==GLFW_PRESS|| glfwGetKey(window, GLFW_KEY_D)==GLFW_PRESS;
    bool enter = glfwGetKey(window, GLFW_KEY_ENTER)==GLFW_PRESS|| glfwGetKey(window, GLFW_KEY_SPACE)==GLFW_PRESS;
    bool esc   = glfwGetKey(window, GLFW_KEY_ESCAPE)==GLFW_PRESS;

    switch (state) {
        case MAIN_MENU:
            if (up && !upPressed)   selectedMenuItem = (selectedMenuItem - 1 + 3) % 3;
            if (down && !downPressed) selectedMenuItem = (selectedMenuItem + 1) % 3;
            if (enter && !enterPressed) {
                if (selectedMenuItem == 0) SetState(DIFFICULTY_MENU);
                else if (selectedMenuItem == 1) SetState(SETTINGS_MENU);
                else if (selectedMenuItem == 2) glfwSetWindowShouldClose(window, true);
            }
            break;
        case DIFFICULTY_MENU:
            if (up && !upPressed)   selectedDifficultyItem = (selectedDifficultyItem - 1 + 2) % 2;
            if (down && !downPressed) selectedDifficultyItem = (selectedDifficultyItem + 1) % 2;
            if (enter && !enterPressed) {
                difficulty = (selectedDifficultyItem == 0) ? EASY : HARD;
                startNewGame();
            }
            if (esc && !escPressed) SetState(MAIN_MENU);
            break;
        case SETTINGS_MENU: {
            const int items = 4; // Rainbow Water, Crazy Physics, Party Mode, Boat Skin
            if (up && !upPressed)   selectedSettingsItem = (selectedSettingsItem - 1 + items) % items;
            if (down && !downPressed) selectedSettingsItem = (selectedSettingsItem + 1) % items;

            if (left && !leftPressed) {
                switch (selectedSettingsItem) {
                    case 0: enableRainbowWater = !enableRainbowWater; break;
                    case 1: enableCrazyPhysics = !enableCrazyPhysics; break;
                    case 2: enablePartyMode    = !enablePartyMode;    break;
                    case 3: boatSkinIndex      = (boatSkinIndex - 1 + BoatSkinId::COUNT) % BoatSkinId::COUNT; break;
                }
            }
            if (right && !rightPressed) {
                switch (selectedSettingsItem) {
                    case 0: enableRainbowWater = !enableRainbowWater; break;
                    case 1: enableCrazyPhysics = !enableCrazyPhysics; break;
                    case 2: enablePartyMode    = !enablePartyMode;    break;
                    case 3: boatSkinIndex      = (boatSkinIndex + 1) % BoatSkinId::COUNT; break;
                }
            }
            if (esc && !escPressed) SetState(MAIN_MENU);
        } break;
        case PAUSED:
            if (up && !upPressed)   selectedPauseItem = (selectedPauseItem - 1 + 3) % 3;
            if (down && !downPressed) selectedPauseItem = (selectedPauseItem + 1) % 3;
            if (enter && !enterPressed) {
                if (selectedPauseItem == 0) { SetState(PLAYING); }
                if (selectedPauseItem == 1) { SetState(SETTINGS_MENU); }
                if (selectedPauseItem == 2) { SetState(MAIN_MENU); }
            }
            if (esc && !escPressed) { SetState(PLAYING); }
            break;
        case GAME_OVER:
            if (enter && !enterPressed) SetState(MAIN_MENU);
            break;
        default: break;
    }

    upPressed=up; downPressed=down; leftPressed=left; rightPressed=right; enterPressed=enter; escPressed=esc;
}

void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (!game || game->GetState() != PLAYING) return;

    if (game->firstMouse) {
        game->lastX = xpos; game->lastY = ypos; game->firstMouse = false;
    }

    float rawX = static_cast<float>(xpos - game->lastX);
    float rawY = static_cast<float>(game->lastY - ypos);
    game->lastX = xpos; game->lastY = ypos;

    const float sensitivity = 0.1f;
    float xoffset = rawX * sensitivity;
    float yoffset = rawY * sensitivity;

    if (game->boatSkinIndex == BoatSkinId::GOING_MERRY) {
        if (game->shipCamera.mode == Camera::FIRST_PERSON) {
            game->GetPlayer()->AdjustRotation(-xoffset);
            float clampedPitch = glm::clamp(game->shipCamera.Pitch + yoffset, -89.0f, 89.0f);
            game->shipCamera.SetPitch(clampedPitch);
        } else {
            game->shipCamera.ProcessMouseMovement(rawX, rawY);
        }
        return;
    }

    if (game->isFirstPerson) {
        if (game->boatSkinIndex == BoatSkinId::BIG_MOM) {
            const float mouseSensitivity = 1.5f;
            game->GetPlayer()->AdjustRotation(-xoffset * mouseSensitivity * 0.1f);
        } else {
            game->GetPlayer()->AdjustRotation(-xoffset);
        }
    } else {
        game->cameraYaw   += xoffset;
        game->cameraPitch += yoffset;
        if (game->cameraPitch >  89.0f) game->cameraPitch =  89.0f;
        if (game->cameraPitch < -89.0f) game->cameraPitch = -89.0f;
    }
}



