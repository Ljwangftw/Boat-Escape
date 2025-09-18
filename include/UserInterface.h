#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include <string>
#include <glm/glm.hpp>
#include "Game.h"

class UserInterface {
public:
    UserInterface();
    ~UserInterface();

    void Init();

    void RenderMenu(GameState state,
                    int selectedMain,
                    int selectedDiff,
                    int selectedSettings,
                    bool rainbowOn,
                    bool crazyOn,
                    bool partyOn,
                    int  skinIndex);

    void RenderHUD(int health, int maxHealth, int score, float gameTime, int enemiesDestroyed, Difficulty difficulty);
    void RenderPauseScreen(int selectedItem);
    void RenderGameOverScreen(int finalScore, int enemiesDestroyed, float gameTime, Difficulty difficulty);

private:
    unsigned int textShaderProgram;
    unsigned int textVAO;

    void setupTextBuffers();
    unsigned int createTextShaderProgram();

    void renderText(const std::string& text, float x, float y, float scale, glm::vec3 color);
    void renderCenteredText(const std::string& text, float y, float scale, glm::vec3 color);
};

#endif 
