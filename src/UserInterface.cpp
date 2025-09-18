#include "UserInterface.h"
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <map>
#include <vector>

const unsigned int WINDOW_WIDTH = 1024;
const unsigned int WINDOW_HEIGHT = 768;

const char* textVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
uniform mat4 projection;
uniform mat4 model;
void main() {
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* textFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform vec3 textColor;
void main() {
    FragColor = vec4(textColor, 1.0);
}
)";

// 5x7 bitmap font 
static std::map<char, std::vector<int>> charBitmaps = {
    {'A',{0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,1,1,1,1,1,0,0,0,1,1,0,0,0,1,0,0,0,0,0}},
    {'B',{1,1,1,1,0,1,0,0,0,1,1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,1,1,1,0,0,0,0,0,0}},
    {'C',{0,1,1,1,0,1,0,0,0,1,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,0,1,1,1,0,0,0,0,0,0}},
    {'D',{1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,1,1,1,0,0,0,0,0,0}},
    {'E',{1,1,1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,1,0,0,0,0,1,1,1,1,1,0,0,0,0,0}},
    {'F',{1,1,1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0}},
    {'G',{0,1,1,1,0,1,0,0,0,1,1,0,0,0,0,1,0,1,1,1,1,0,0,0,1,0,0,1,1,0,0,0,0,0,0}},
    {'H',{1,0,0,0,1,1,0,0,0,1,1,1,1,1,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,0,0,0,0,0}},
    {'I',{0,1,1,1,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,1,1,0,0,0,0,0,0}},
    {'J',{0,0,0,1,1,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0}},
    {'K',{1,0,0,0,1,1,0,0,1,0,1,0,1,0,0,1,1,0,0,1,0,1,0,0,1,1,0,0,0,1,0,0,0,0,0}},
    {'L',{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,1,1,1,1,0,0,0,0,0}},
    {'M',{1,0,0,0,1,1,1,0,1,1,1,0,1,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,0,0,0,0,0}},
    {'N',{1,0,0,0,1,1,1,0,0,1,1,0,1,0,1,1,0,0,1,1,1,0,0,0,1,1,0,0,0,1,0,0,0,0,0}},
    {'O',{0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0,0,0,0,0,0}},
    {'P',{1,1,1,1,0,1,0,0,0,1,1,1,1,1,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0}},
    {'Q',{0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,1,1,0,0,0,0,0}},
    {'R',{1,1,1,1,0,1,0,0,0,1,1,1,1,1,0,1,0,1,0,0,1,0,0,1,0,1,0,0,0,1,0,0,0,0,0}},
    {'S',{0,1,1,1,0,1,0,0,0,0,0,1,1,0,0,0,0,0,1,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0}},
    {'T',{1,1,1,1,1,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,0,0}},
    {'U',{1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0,0,0,0,0,0}},
    {'V',{1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,0,1,0,1,0,0,1,0,1,0,0,0,1,0,0,0,0,0,0,0}},
    {'W',{1,0,0,0,1,1,0,0,0,1,1,0,1,0,1,1,0,1,0,1,1,1,0,1,1,1,0,0,0,1,0,0,0,0,0}},
    {'X',{1,0,0,0,1,0,1,0,1,0,0,1,1,0,0,1,0,0,1,0,1,0,1,0,1,1,0,0,0,1,0,0,0,0,0}},
    {'Y',{1,0,0,0,1,1,0,0,0,1,0,1,0,1,0,0,0,1,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0}},
    {'Z',{1,1,1,1,1,0,0,0,1,0,0,0,1,0,0,1,0,0,1,0,0,0,1,0,0,1,1,1,1,1,0,0,0,0,0}},
    {' ',{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
    {':',{0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0}},
    {'/',{0,0,0,0,1,0,0,0,1,0,0,1,0,0,0,1,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0}},
    {'0',{0,1,1,1,0,1,0,0,0,1,1,0,0,1,1,1,0,1,0,1,1,0,0,1,1,0,1,1,1,0,0,0,0,0,0}},
    {'1',{0,0,1,0,0,0,1,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,1,1,0,0,0,0,0,0}},
    {'2',{0,1,1,1,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,1,1,1,1,1,0,0,0,0,0,0}},
    {'3',{1,1,1,1,0,0,0,0,0,1,0,1,1,1,0,0,0,0,0,1,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0}},
    {'4',{1,0,0,0,1,1,0,0,0,1,1,1,1,1,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0}},
    {'5',{1,1,1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0,0,1,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0}},
    {'6',{0,1,1,1,0,1,0,0,0,0,1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0,0,0,0,0,0}},
    {'7',{1,1,1,1,1,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0}},
    {'8',{0,1,1,1,0,1,0,0,0,1,0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0,0,0,0,0,0}},
    {'9',{0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,0,1,1,1,1,0,0,0,1,0,0,1,1,1,0,0,0,0,0,0}}
};

UserInterface::UserInterface() : textShaderProgram(0), textVAO(0) {}
UserInterface::~UserInterface() {
    glDeleteVertexArrays(1, &textVAO);
    glDeleteProgram(textShaderProgram);
}

void UserInterface::Init() {
    textShaderProgram = createTextShaderProgram();
    setupTextBuffers();
}

void UserInterface::RenderMenu(GameState state, int selectedMain, int selectedDiff, int selectedSettings,
                               bool rainbow, bool physics, bool party, int skin) {
    glDisable(GL_DEPTH_TEST);
    if (state == MAIN_MENU) {
        renderCenteredText("BOAT ESCAPE", 550.0f, 3.0f, glm::vec3(1.0f, 1.0f, 0.0f));
        glm::vec3 normalColor = glm::vec3(0.8f);
        glm::vec3 selectedColor = glm::vec3(1.0f, 1.0f, 0.0f);
        renderCenteredText("START",    420.0f, 2.0f, selectedMain == 0 ? selectedColor : normalColor);
        renderCenteredText("SETTINGS", 370.0f, 2.0f, selectedMain == 1 ? selectedColor : normalColor);
        renderCenteredText("QUIT",     320.0f, 2.0f, selectedMain == 2 ? selectedColor : normalColor);
        renderCenteredText("USE W/S TO SELECT", 250.0f, 1.2f, glm::vec3(0.6f));
        renderCenteredText("PRESS ENTER",        220.0f, 1.2f, glm::vec3(0.6f));
    } else if (state == DIFFICULTY_MENU) {
        renderCenteredText("SELECT DIFFICULTY", 550.0f, 2.5f, glm::vec3(1.0f, 0.5f, 0.0f));
        glm::vec3 normalColor = glm::vec3(0.8f);
        glm::vec3 selectedColor = glm::vec3(1.0f, 1.0f, 0.0f);
        renderCenteredText("EASY", 450.0f, 2.5f, selectedDiff == 0 ? selectedColor : normalColor);
        renderCenteredText("HARD", 330.0f, 2.5f, selectedDiff == 1 ? selectedColor : normalColor);
        renderCenteredText("USE W/S TO SELECT", 200.0f, 1.2f, glm::vec3(0.6f));
        renderCenteredText("PRESS ENTER",        170.0f, 1.2f, glm::vec3(0.6f));
        renderCenteredText("ESC TO GO BACK",     140.0f, 1.2f, glm::vec3(0.6f));
    } else if (state == SETTINGS_MENU) {
        renderCenteredText("SETTINGS", 600.0f, 2.5f, glm::vec3(0.0f, 1.0f, 1.0f));
        glm::vec3 normalColor = glm::vec3(0.8f);
        glm::vec3 selectedColor = glm::vec3(0.0f, 1.0f, 1.0f);
        const char* boatSkins[] = {"THOUSAND SUNNY", "BLACK BEARD", "GOL D ROGER", "BUGGY CLOWN", "BIG MOM", "GOING MERRY"};

        // Options
        renderText("RAINBOW WATER: " + std::string(rainbow ? "ON" : "OFF"), 200.0f, 520.0f, 1.5f, selectedSettings == 0 ? selectedColor : normalColor);
        renderText("CRAZY PHYSICS: " + std::string(physics ? "ON" : "OFF"), 200.0f, 480.0f, 1.5f, selectedSettings == 1 ? selectedColor : normalColor);
        renderText("PARTY MODE: "     + std::string(party   ? "ON" : "OFF"), 200.0f, 440.0f, 1.5f, selectedSettings == 2 ? selectedColor : normalColor);
        int safeSkin = skin; if (safeSkin < 0) safeSkin = 0; if (safeSkin >= static_cast<int>(sizeof(boatSkins)/sizeof(boatSkins[0]))) safeSkin = static_cast<int>(sizeof(boatSkins)/sizeof(boatSkins[0])) - 1;
        renderText(std::string("BOAT SKIN: ") + boatSkins[safeSkin],           200.0f, 400.0f, 1.5f, selectedSettings == 3 ? selectedColor : normalColor);

        renderCenteredText("USE A/D TO CHANGE", 200.0f, 1.2f, glm::vec3(0.6f));
        renderCenteredText("ESC TO GO BACK",    170.0f, 1.2f, glm::vec3(0.6f));
    }
    glEnable(GL_DEPTH_TEST);
}

void UserInterface::RenderHUD(int health, int maxHealth, int score, float gameTime, int enemiesDestroyed, Difficulty difficulty) {
    glDisable(GL_DEPTH_TEST);
    renderText("HEALTH: " + std::to_string(health) + "/" + std::to_string(maxHealth), 20.0f, WINDOW_HEIGHT - 70.0f, 1.2f, glm::vec3(1.0f));
    renderText("SCORE: " + std::to_string(score), 20.0f, WINDOW_HEIGHT - 100.0f, 1.2f, glm::vec3(1.0f, 1.0f, 0.0f));
    renderText("TIME: " + std::to_string((int)gameTime) + "S", 20.0f, WINDOW_HEIGHT - 130.0f, 1.2f, glm::vec3(0.7f, 0.7f, 1.0f));
    renderText("ENEMIES: " + std::to_string(enemiesDestroyed), 20.0f, WINDOW_HEIGHT - 160.0f, 1.2f, glm::vec3(1.0f, 0.5f, 0.5f));
    std::string difficultyText = "DIFFICULTY: " + std::string(difficulty == EASY ? "EASY" : "HARD");
    renderText(difficultyText, 20.0f, WINDOW_HEIGHT - 190.0f, 1.2f, glm::vec3(0.9f, 0.6f, 0.9f));
    glEnable(GL_DEPTH_TEST);
}

void UserInterface::RenderPauseScreen(int selectedItem) {
    glDisable(GL_DEPTH_TEST);
    renderCenteredText("GAME PAUSED", 500.0f, 3.0f, glm::vec3(1.0f, 1.0f, 0.0f));
    glm::vec3 normalColor = glm::vec3(0.9f);
    glm::vec3 selectedColor = glm::vec3(1.0f, 1.0f, 0.0f);
    renderCenteredText("CONTINUE",           400.0f, 2.0f, selectedItem == 0 ? selectedColor : normalColor);
    renderCenteredText("SETTINGS",           350.0f, 2.0f, selectedItem == 1 ? selectedColor : normalColor);
    renderCenteredText("EXIT TO MAIN MENU",  300.0f, 2.0f, selectedItem == 2 ? selectedColor : normalColor);
    glEnable(GL_DEPTH_TEST);
}

void UserInterface::RenderGameOverScreen(int finalScore, int enemiesDestroyed, float gameTime, Difficulty difficulty) {
    glClearColor(0.3f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    renderCenteredText("GAME OVER", 550.0f, 3.0f, glm::vec3(1.0f, 0.2f, 0.2f));
    renderCenteredText("FINAL SCORE: " + std::to_string(finalScore), 450.0f, 2.0f, glm::vec3(1.0f, 1.0f, 0.0f));
    renderCenteredText("ENEMIES DESTROYED: " + std::to_string(enemiesDestroyed), 400.0f, 1.8f, glm::vec3(0.9f));
    renderCenteredText("SURVIVAL TIME: " + std::to_string((int)gameTime) + " SECONDS", 360.0f, 1.8f, glm::vec3(0.7f, 0.9f, 1.0f));
    std::string difficultyText = "DIFFICULTY: " + std::string(difficulty == EASY ? "EASY" : "HARD");
    renderCenteredText(difficultyText, 320.0f, 1.8f, glm::vec3(0.9f, 0.6f, 0.9f));
    renderCenteredText("PRESS ENTER TO RETURN TO MAIN MENU", 150.0f, 1.3f, glm::vec3(0.8f));

    glEnable(GL_DEPTH_TEST);
}

void UserInterface::setupTextBuffers() {
    float textVertices[] = {
        0.0f,1.0f, 0.0f,1.0f, 1.0f,0.0f, 1.0f,0.0f, 0.0f,0.0f, 0.0f,0.0f,
        0.0f,1.0f, 0.0f,1.0f, 1.0f,1.0f, 1.0f,1.0f, 1.0f,0.0f, 1.0f,0.0f
    };
    unsigned int textVBO;
    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(textVertices), textVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

unsigned int UserInterface::createTextShaderProgram() {
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &textVertexShaderSource, NULL);
    glCompileShader(vs);

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &textFragmentShaderSource, NULL);
    glCompileShader(fs);

    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

void UserInterface::renderText(const std::string& text, float x, float y, float scale, glm::vec3 color) {
    glUseProgram(textShaderProgram);
    glm::mat4 projection = glm::ortho(0.0f, (float)WINDOW_WIDTH, 0.0f, (float)WINDOW_HEIGHT);
    glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniform3fv(glGetUniformLocation(textShaderProgram, "textColor"), 1, &color[0]);
    glBindVertexArray(textVAO);

    float pixelSize = 3.0f * scale;
    float charWidth = 5.0f * pixelSize;

    for (size_t i = 0; i < text.length(); i++) {
        char c = toupper(text[i]);
        auto it = charBitmaps.find(c);
        if (it == charBitmaps.end()) continue;
        const std::vector<int>& bitmap = it->second;

        for (int row = 0; row < 7; row++) {
            for (int col = 0; col < 5; col++) {
                if (bitmap[row*5 + col] == 1) {
                    float xpos = x + i * (charWidth + pixelSize) + col * pixelSize;
                    float ypos = y + (6 - row) * pixelSize;
                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(xpos, ypos, 0.0f));
                    model = glm::scale(model, glm::vec3(pixelSize, pixelSize, 1.0f));
                    glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
            }
        }
    }
    glBindVertexArray(0);
}

void UserInterface::renderCenteredText(const std::string& text, float y, float scale, glm::vec3 color) {
    float pixelSize = 3.0f * scale;
    float charWidth = 5.0f * pixelSize;
    float totalWidth = text.length() * (charWidth + pixelSize) - pixelSize;
    float x = (WINDOW_WIDTH - totalWidth) / 2.0f;
    renderText(text, x, y, scale, color);
}
