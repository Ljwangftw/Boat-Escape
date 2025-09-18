#ifndef GAME_H
#define GAME_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include "Camera.h"
#include "BoatSkinIds.h"

class Graphics;
class Player;
class EnemyManager;
class ProjectileManager;
class MountainManager;
class UserInterface;

enum GameState { MAIN_MENU, DIFFICULTY_MENU, SETTINGS_MENU, PLAYING, PAUSED, GAME_OVER };
enum Difficulty { EASY, HARD };

class Game {
public:
    Game(unsigned int width, unsigned int height);
    ~Game();

    void Init();
    void ProcessInput(float dt);
    void Update(float dt);
    void Render();

    inline bool IsRunning() const { return window && !glfwWindowShouldClose(window); }
    inline GLFWwindow* GetWindow() const { return window; }

    inline GameState GetState() const { return state; }
    void SetState(GameState s);

public:
    bool  firstMouse;
    double lastX, lastY;
    bool  isFirstPerson;
    Camera shipCamera;

    float cameraYaw;
    float cameraPitch;
    float cameraDistance;
    float cameraHeight;

    bool enableRainbowWater;
    bool enableCrazyPhysics;
    bool enablePartyMode;
    bool enableDebugMountains = false;

    int boatSkinIndex;

    Player* GetPlayer() { return player.get(); }

private:
    void startNewGame();
    void triggerGameOver();
    void checkCollisions();
    void ProcessMenuInput(float dt);

private:
    GLFWwindow* window;
    unsigned int screenWidth, screenHeight;

    std::unique_ptr<Graphics>          graphics;
    std::unique_ptr<Player>            player;
    std::unique_ptr<EnemyManager>      enemyManager;
    std::unique_ptr<ProjectileManager> projectileManager;
    std::unique_ptr<MountainManager>   mountainManager;
    std::unique_ptr<UserInterface>     ui;

    GameState  state;
    Difficulty difficulty;

    float gameTime;
    int   score;
    int   enemiesDestroyed;
    int   finalScore;

    int selectedMenuItem;
    int selectedDifficultyItem;
    int selectedSettingsItem;
    int selectedPauseItem;

    float waveTime;

    float musicVolume = 0.7f;
    bool  enableScreenShake = true;
    bool  enableUnicornMode = false;
    float gameSpeed = 1.0f;
};

#endif 


