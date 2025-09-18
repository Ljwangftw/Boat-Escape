#include "../include/Game.h"

const unsigned int WINDOW_WIDTH = 1024;
const unsigned int WINDOW_HEIGHT = 768;

int main() {
    Game game(WINDOW_WIDTH, WINDOW_HEIGHT);
    game.Init();

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    while (game.IsRunning()) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        game.ProcessInput(deltaTime);
        game.Update(deltaTime);
        game.Render();

        glfwSwapBuffers(game.GetWindow());
        
        glfwPollEvents();
    }

    return 0;
}

