#include "Graphics.h"
#include "BoatSkinIds.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#include <iostream>
#include <vector>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static constexpr float WATER_LEVEL = -1.0f;
static constexpr float WATER_HALF  = 500.0f;

// Add Big Mom as a dedicated 5th skin
enum BoatSkin { SKIN_SUNNY=0, SKIN_BLACKBEARD=1, SKIN_GOL_D_ROGER=2, SKIN_BUGGY=3, SKIN_BIGMOM=4, SKIN_GOING_MERRY=5, SKIN_COUNT=6 };

struct FPCamPreset { float height, forward, lookAhead, extraPitchDeg, lateral; };
static FPCamPreset kFPCamBySkin[SKIN_COUNT] = {
    { 2.3f, 1.30f, 6.0f, -4.0f, 0.00f }, // Sunny
    { 2.3f, 1.90f, 6.0f, -2.0f, 0.00f }, // Blackbeard
    { 2.3f, 1.10f, 6.0f, -2.0f, 0.07f }, // Gol D Roger
    { 1.1f, 1.25f, 6.0f, -2.0f, 0.15f }, // Buggy
    { 2.0f, 0.80f, 6.0f, -2.0f, 0.00f }  // Big Mom (unused; Big Mom uses custom logic below)
};

static inline int GetSafeSkinIndex(int idx){ return (idx < 0 || idx >= SKIN_COUNT) ? 0 : idx; }

static float kBoatWaterlineBySkin[SKIN_COUNT] = { 0.95f, 2.6f, 1.9f, 1.4f, 2.2f, 0.25f };
static float kEnemyWaterlineOffset = 0.39f;

static const float kCubeVerts[] = {
    // pos                // normal
    // front
    -0.5f,-0.5f, 0.5f,    0,0,1,  0.5f,-0.5f, 0.5f,    0,0,1,  0.5f, 0.5f, 0.5f,    0,0,1,
     0.5f, 0.5f, 0.5f,    0,0,1, -0.5f, 0.5f, 0.5f,    0,0,1, -0.5f,-0.5f, 0.5f,    0,0,1,
    // back
     0.5f,-0.5f,-0.5f,    0,0,-1, -0.5f,-0.5f,-0.5f,   0,0,-1, -0.5f, 0.5f,-0.5f,   0,0,-1,
    -0.5f, 0.5f,-0.5f,    0,0,-1,  0.5f, 0.5f,-0.5f,   0,0,-1,  0.5f,-0.5f,-0.5f,   0,0,-1,
    // left
    -0.5f,-0.5f,-0.5f,   -1,0,0,  -0.5f,-0.5f, 0.5f,  -1,0,0,  -0.5f, 0.5f, 0.5f,  -1,0,0,
    -0.5f, 0.5f, 0.5f,   -1,0,0,  -0.5f, 0.5f,-0.5f,  -1,0,0,  -0.5f,-0.5f,-0.5f,  -1,0,0,
    // right
     0.5f,-0.5f, 0.5f,    1,0,0,   0.5f,-0.5f,-0.5f,   1,0,0,   0.5f, 0.5f,-0.5f,   1,0,0,
     0.5f, 0.5f,-0.5f,    1,0,0,   0.5f, 0.5f, 0.5f,   1,0,0,   0.5f,-0.5f, 0.5f,   1,0,0,
    // top
    -0.5f, 0.5f, 0.5f,    0,1,0,   0.5f, 0.5f, 0.5f,   0,1,0,   0.5f, 0.5f,-0.5f,   0,1,0,
     0.5f, 0.5f,-0.5f,    0,1,0,  -0.5f, 0.5f,-0.5f,   0,1,0,  -0.5f, 0.5f, 0.5f,   0,1,0,
    // bottom
    -0.5f,-0.5f,-0.5f,    0,-1,0,  0.5f,-0.5f,-0.5f,   0,-1,0,  0.5f,-0.5f, 0.5f,   0,-1,0,
     0.5f,-0.5f, 0.5f,    0,-1,0, -0.5f,-0.5f, 0.5f,   0,-1,0, -0.5f,-0.5f,-0.5f,   0,-1,0,
};

static float kWaterVerts[] = {
    -WATER_HALF, WATER_LEVEL, -WATER_HALF,  0,1,0,
     WATER_HALF, WATER_LEVEL, -WATER_HALF,  0,1,0,
     WATER_HALF, WATER_LEVEL,  WATER_HALF,  0,1,0,

    -WATER_HALF, WATER_LEVEL, -WATER_HALF,  0,1,0,
     WATER_HALF, WATER_LEVEL,  WATER_HALF,  0,1,0,
    -WATER_HALF, WATER_LEVEL,  WATER_HALF,  0,1,0,
};

static const float kSkyboxVerts[] = {
    -1,-1,-1,  1,-1,-1,  1, 1,-1,   1, 1,-1, -1, 1,-1, -1,-1,-1,
    -1,-1, 1,  1,-1, 1,  1, 1, 1,   1, 1, 1, -1, 1, 1, -1,-1, 1,
    -1, 1, 1, -1, 1,-1, -1,-1,-1,  -1,-1,-1, -1,-1, 1, -1, 1, 1,
     1, 1, 1,  1, 1,-1,  1,-1,-1,   1,-1,-1,  1,-1, 1,  1, 1, 1,
    -1,-1,-1,  1,-1,-1,  1,-1, 1,   1,-1, 1, -1,-1, 1, -1,-1,-1,
    -1, 1,-1,  1, 1,-1,  1, 1, 1,   1, 1, 1, -1, 1, 1, -1, 1,-1
};

Graphics::Graphics()
    : shaderProgram(0),
      shipVAO(0), waterVAO(0), skyboxVAO(0), cubeVAO(0),
      mountainVAO(0), mountainVBO(0), mountainEBO(0),
      sphereVAO(0), sphereVBO(0), sphereEBO(0),
      debugVAO(0), debugVBO(0) {}

Graphics::~Graphics() {
    glDeleteVertexArrays(1, &shipVAO);
    glDeleteVertexArrays(1, &waterVAO);
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &skyboxVAO);

    glDeleteVertexArrays(1, &mountainVAO);
    glDeleteBuffers(1, &mountainVBO);
    glDeleteBuffers(1, &mountainEBO);

    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);

    if (!mountainTextures.empty())
        glDeleteTextures((GLsizei)mountainTextures.size(), mountainTextures.data());

    if (debugVAO) glDeleteVertexArrays(1, &debugVAO);
    if (debugVBO) glDeleteBuffers(1, &debugVBO);

    if (shaderProgram) glDeleteProgram(shaderProgram);
    if (goingMerryShader) glDeleteProgram(goingMerryShader);
}

void Graphics::Init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shaderProgram = createShaderProgram();
    glUseProgram(shaderProgram);

    const char* gmVertexSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
out vec2 TexCoords;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
    TexCoords = aTexCoords;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";
    const char* gmFragmentSrc = R"(#version 330 core
out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D texture_diffuse1;
uniform bool use_texture;
void main() {
    if (use_texture) {
        FragColor = texture(texture_diffuse1, TexCoords);
    } else {
        FragColor = vec4(0.2, 0.2, 0.2, 1.0);
    }
}
)";
    GLuint gmVS = compileShader(GL_VERTEX_SHADER, gmVertexSrc);
    GLuint gmFS = compileShader(GL_FRAGMENT_SHADER, gmFragmentSrc);
    goingMerryShader = glCreateProgram();
    glAttachShader(goingMerryShader, gmVS);
    glAttachShader(goingMerryShader, gmFS);
    glLinkProgram(goingMerryShader);
    glDeleteShader(gmVS);
    glDeleteShader(gmFS);
    setupBuffers();
    setupMountainBuffers();

    mountainTextures.push_back(loadTexture("texture/ice and snow.png"));
    mountainTextures.push_back(loadTexture("texture/burning-hot-lava.png"));
    mountainTextures.push_back(loadTexture("texture/green-grass.png"));
    mountainTextures.push_back(loadTexture("texture/ground-rocks.png"));
    mountainTextures.push_back(loadTexture("texture/sandstone.png"));

    modelManager = std::make_unique<ModelManager>();
    modelManager->LoadAllBoatModels();
}

void Graphics::Render(const Player& player,
                      const EnemyManager& enemyManager,
                      const ProjectileManager& projectileManager,
                      const MountainManager& mountainManager,
                      float waveTime,
                      float cameraYaw,
                      float cameraPitch,
                      float cameraDistance,
                      float cameraHeight,
                      bool isFirstPerson,
                      bool enableRainbowWater,
                      bool enablePartyModeForPlayer,
                      int  boatSkinIndex,
                      bool debugMountains,
                      Camera& shipCamera,
                      const glm::vec3& shipFront) {
    glClearColor(0.58f, 0.82f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);

    // camera
    glm::mat4 view(1.0f);
    glm::vec3 viewPos(0.0f);
    const glm::vec3 playerPos = player.GetPosition();

    const int skin = GetSafeSkinIndex(boatSkinIndex);
    glm::vec3 basePos = playerPos;
    if (skin == SKIN_BIGMOM) {
        basePos.y = WATER_LEVEL + kBoatWaterlineBySkin[skin];
    }

    bool isGoingMerry = (boatSkinIndex == BoatSkinId::GOING_MERRY);

    if (isGoingMerry) {
        float shipYawRad = glm::radians(player.GetRotation());
        glm::mat4 gmView = shipCamera.GetViewMatrix(playerPos, shipFront, shipYawRad);
        view    = gmView;
        viewPos = shipCamera.Position;
    } else if (isFirstPerson) {
        float yawRad = glm::radians(player.GetRotation());
        glm::vec3 forward = glm::normalize(glm::vec3(std::sin(yawRad), 0.0f, std::cos(yawRad)));
        glm::vec3 right   = glm::normalize(glm::cross(forward, glm::vec3(0,1,0)));

        if (skin == SKIN_BIGMOM) {
            glm::vec3 modelUp = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 localOffset = forward * 1.91f + modelUp * -0.65f;
            glm::vec3 eye = basePos + localOffset;

            glm::vec3 front_with_pitch = forward;
            front_with_pitch.y = sin(glm::radians(cameraPitch));
            float horizontal_scale = cos(glm::radians(cameraPitch));
            front_with_pitch.x *= horizontal_scale;
            front_with_pitch.z *= horizontal_scale;

            view    = glm::lookAt(eye, eye + glm::normalize(front_with_pitch), glm::vec3(0,1,0));
            viewPos = eye;
        } else {
            const FPCamPreset cfg = kFPCamBySkin[skin];
            glm::vec3 eye = basePos + glm::vec3(0.0f, cfg.height, 0.0f) + forward*cfg.forward + right*cfg.lateral;

            float pitchRad = glm::radians(cfg.extraPitchDeg);
            glm::vec3 viewDir = glm::normalize(forward * std::cos(pitchRad) + glm::vec3(0,1,0) * std::sin(pitchRad));

            view    = glm::lookAt(eye, eye + viewDir * cfg.lookAhead, glm::vec3(0,1,0));
            viewPos = eye;
        }
    } else {
        float yawRad   = glm::radians(cameraYaw);
        float pitchRad = glm::radians(glm::clamp(cameraPitch, -89.0f, 89.0f));
        float r        = cameraDistance;

        float offX = r * std::cos(pitchRad) * std::sin(yawRad);
        float offY = r * std::sin(pitchRad);
        float offZ = r * std::cos(pitchRad) * std::cos(yawRad);

        glm::vec3 cameraPos = basePos + glm::vec3(-offX, cameraHeight + offY, -offZ);
        glm::vec3 target    = basePos + glm::vec3(0.0f, 0.3f, 0.0f);

        view    = glm::lookAt(cameraPos, target, glm::vec3(0,1,0));
        viewPos = cameraPos;
    }

    // projection
    GLint vp[4]; glGetIntegerv(GL_VIEWPORT, vp);
    float aspect = (vp[3] > 0) ? (float)vp[2] / (float)vp[3] : (1024.0f/768.0f);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 2000.0f);

    // common uniforms
    const GLint viewLoc        = glGetUniformLocation(shaderProgram, "view");
    const GLint projLoc        = glGetUniformLocation(shaderProgram, "projection");
    const GLint viewPosLoc     = glGetUniformLocation(shaderProgram, "viewPos");
    const GLint lightPosLoc    = glGetUniformLocation(shaderProgram, "lightPos");
    const GLint lightColLoc    = glGetUniformLocation(shaderProgram, "lightColor");
    const GLint timeLoc        = glGetUniformLocation(shaderProgram, "time");
    const GLint isWaterLoc     = glGetUniformLocation(shaderProgram, "isWater");
    const GLint useTexLoc      = glGetUniformLocation(shaderProgram, "useTexture");
    const GLint objColorLoc    = glGetUniformLocation(shaderProgram, "objectColor");
    const GLint invertVLoc     = glGetUniformLocation(shaderProgram, "invertV");
    const GLint exposureLoc    = glGetUniformLocation(shaderProgram, "exposure");
    const GLint ambientBoostL  = glGetUniformLocation(shaderProgram, "ambientBoost");
    const GLint waterCenterLoc = glGetUniformLocation(shaderProgram, "waterCenter");
    const GLint waterAmpLoc    = glGetUniformLocation(shaderProgram, "waterAmp");
    const GLint waterSpeedLoc  = glGetUniformLocation(shaderProgram, "waterSpeed");
    const GLint rainbowLoc     = glGetUniformLocation(shaderProgram, "rainbowWater");
    const GLint partyModeLoc   = glGetUniformLocation(shaderProgram, "partyMode");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
    if (goingMerryShader) {
        glUseProgram(goingMerryShader);
        GLint gmProj = glGetUniformLocation(goingMerryShader, "projection");
        GLint gmView = glGetUniformLocation(goingMerryShader, "view");
        if (gmProj >= 0) glUniformMatrix4fv(gmProj, 1, GL_FALSE, &projection[0][0]);
        if (gmView >= 0) glUniformMatrix4fv(gmView, 1, GL_FALSE, &view[0][0]);
        glUseProgram(shaderProgram);
    }
    glUniform3fv(viewPosLoc, 1, &viewPos[0]);

    glUniform3f(lightPosLoc, 50.0f, 90.0f, 30.0f);
    glUniform3f(lightColLoc, 1.0f, 0.98f, 0.9f);
    glUniform1f(timeLoc, waveTime);

    glUniform1f(exposureLoc,   1.35f);
    glUniform1f(ambientBoostL, 1.12f);

    glUniform2f(waterCenterLoc, viewPos.x, viewPos.z);
    glUniform1f(waterAmpLoc,    0.10f);
    glUniform1f(waterSpeedLoc,  1.0f);
    glUniform1i(rainbowLoc, enableRainbowWater ? 1 : 0);

    // skybox
    drawSkybox(playerPos);

    // water
    glUniform1i(isWaterLoc, 1);
    glUniform1i(useTexLoc,  0);
    glUniform1i(invertVLoc, 0);
    drawWater(playerPos, waveTime, enableRainbowWater, enablePartyModeForPlayer);
    glUniform1i(isWaterLoc, 0);

    // mountains
    for (const auto& m : mountainManager.GetMountains()) {
        if (!m.active) continue;
        glUniform1i(useTexLoc, 1);
        glUniform1i(invertVLoc, 0);
        glUniform3f(objColorLoc, 1.0f, 1.0f, 1.0f);
        glUniform1i(partyModeLoc, 0);
        drawMountain(m.position, m.scale, glm::vec3(1.0f), m.textureIndex);
        if (debugMountains) {
            drawXZCircle(glm::vec3(m.position.x, WATER_LEVEL + 0.02f, m.position.z),
                         m.scale.x * 3.0f,
                         glm::vec3(1.0f, 0.25f, 0.25f));
        }
    }

    // boats
    glm::vec3 standardBoatScale(5.0f);

    // player boat 
    glm::vec3 playerBoatPosition = playerPos;
    if (skin != SKIN_GOING_MERRY) {
        playerBoatPosition.y = WATER_LEVEL + kBoatWaterlineBySkin[skin];
    }

    if (skin == SKIN_GOING_MERRY) {
        GLint gmUseTex = glGetUniformLocation(goingMerryShader, "use_texture");
        GLint gmSampler = glGetUniformLocation(goingMerryShader, "texture_diffuse1");
        if (gmSampler >= 0) glUniform1i(gmSampler, 0);
        if (gmUseTex >= 0) glUniform1i(gmUseTex, 1);
        glUseProgram(goingMerryShader);
        modelManager->DrawPlayerBoat(goingMerryShader, boatSkinIndex, playerBoatPosition, player.GetRotation(), standardBoatScale);
        glUseProgram(shaderProgram);
    } else {
        glUniform1i(useTexLoc, 1);
        glUniform1i(invertVLoc, modelManager && modelManager->ShouldFlipVForSkin(boatSkinIndex) ? 1 : 0);
        glUniform1i(partyModeLoc, enablePartyModeForPlayer ? 1 : 0);
        glUniform3f(objColorLoc, 1.0f, 1.0f, 1.0f);
        modelManager->DrawPlayerBoat(shaderProgram, boatSkinIndex, playerBoatPosition, player.GetRotation(), standardBoatScale);
    }

    // enemies 
    glUniform1i(partyModeLoc, 0);
    for (const auto& e : enemyManager.GetEnemies()) {
        if (!e.active) continue;
        glm::vec3 enemyPos = e.position;
        enemyPos.y = WATER_LEVEL + kEnemyWaterlineOffset;

        glUniform1i(useTexLoc, 1);
        glUniform1i(invertVLoc, modelManager && modelManager->ShouldFlipVEnemy() ? 1 : 0);
        modelManager->DrawEnemyBoat(shaderProgram, enemyPos, e.rotation, standardBoatScale);
    }

    // ---- Projectiles ----
    const float kPlayerCannonballScale = 0.15f; 
    const float kEnemyCannonballScale  = 0.12f; 

    for (const auto& p : projectileManager.GetProjectiles()) {
    if (!p.active) continue;

    glUniform1i(useTexLoc, 1);
    glUniform1i(invertVLoc, 0);

    const float s = p.isPlayerOwned ? kPlayerCannonballScale : kEnemyCannonballScale;

    if (modelManager) {
        modelManager->DrawCannonball(shaderProgram, p.position, s);
    }

    if (!p.isPlayerOwned) {
        glm::vec3 smokeCol(0.5f);
        glUniform1i(useTexLoc, 0);
        for (const auto& sPos : p.smokeTrail) {
            glUniform3fv(objColorLoc, 1, &smokeCol[0]);
            drawCube(sPos, 0.0f, glm::vec3(0.12f), smokeCol);
        }
    }
}

}

void Graphics::setupBuffers() {
    // Cube
    GLuint cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kCubeVerts), kCubeVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    // Water
    GLuint waterVBO;
    glGenVertexArrays(1, &waterVAO);
    glGenBuffers(1, &waterVBO);
    glBindVertexArray(waterVAO);
    glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kWaterVerts), kWaterVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    // Skybox
    GLuint skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kSkyboxVerts), kSkyboxVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Debug simple position-only buffer (for line loops, etc.)
    glGenVertexArrays(1, &debugVAO);
    glGenBuffers(1, &debugVBO);
    glBindVertexArray(debugVAO);
    glBindBuffer(GL_ARRAY_BUFFER, debugVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Graphics::setupMountainBuffers() {
    std::vector<float> vertices;
    mountainIndices.clear();

    generateHalfSphere(3.0f, 32, 20, vertices, mountainIndices);

    glGenVertexArrays(1, &mountainVAO);
    glGenBuffers(1, &mountainVBO);
    glGenBuffers(1, &mountainEBO);

    glBindVertexArray(mountainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mountainVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mountainEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mountainIndices.size()*sizeof(unsigned int), mountainIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

unsigned int Graphics::loadTexture(const char* path) {
    unsigned int tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    int w=0,h=0,n=0;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &w, &h, &n, 0);
    if (data) {
        GLenum fmt = (n==1)?GL_RED:((n==3)?GL_RGB:GL_RGBA);
        glTexImage2D(GL_TEXTURE_2D, 0, (GLint)fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    } else {
        std::cerr << "Failed to load texture: " << path << "\n";
        unsigned char white[4] = {255,255,255,255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1,1,0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    }
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

unsigned int Graphics::compileShader(unsigned int type, const char* source) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &source, nullptr);
    glCompileShader(s);
    GLint ok = GL_FALSE; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len=0; glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetShaderInfoLog(s, len, nullptr, log.data());
        std::cerr << "Shader compilation failed: " << log << std::endl;
    }
    return s;
}

unsigned int Graphics::createShaderProgram() {
    // Vertex shader
    const char* kVertexSrc = R"(#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;
uniform int isWater;

uniform vec2  waterCenter;
uniform float waterAmp;
uniform float waterSpeed;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

const float WATER_LEVEL = -1.0;

void main() {
    vec3 pos = aPos;
    vec3 nrm = aNormal;

    if (isWater == 1) {
        pos.x += waterCenter.x;
        pos.z += waterCenter.y;
        pos.y  = WATER_LEVEL;

        float t = time * waterSpeed;

        float f1 = 0.06; float a1 = 1.2;
        float f2 = 0.10; float a2 = 0.6;
        float f3 = 0.03; float a3 = 0.4;

        float h    = a1 * sin(pos.x*f1 + t*1.3)
                   + a2 * sin(pos.z*f2 + t*1.7)
                   + a3 * sin((pos.x+pos.z)*f3 + t*0.9);
        float dhdx = a1*f1 * cos(pos.x*f1 + t*1.3)
                   + a3*f3 * cos((pos.x+pos.z)*f3 + t*0.9);
        float dhdz = a2*f2 * cos(pos.z*f2 + t*1.7)
                   + a3*f3 * cos((pos.x+pos.z)*f3 + t*0.9);

        h    *= waterAmp;
        dhdx *= waterAmp;
        dhdz *= waterAmp;

        pos.y += h;
        nrm = normalize(vec3(-dhdx, 1.0, -dhdz));
    }

    mat3 normalMat = mat3(transpose(inverse(model)));
    Normal  = normalize(normalMat * nrm);
    FragPos = vec3(model * vec4(pos, 1.0));
    TexCoord = aTexCoord;

    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

    // Fragment shader
    const char* kFragmentSrc = R"(#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec3 lightColor;

uniform int  isWater;
uniform int  useTexture;
uniform vec3 objectColor;  // used as tint in party mode
uniform int  invertV;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

uniform float time;

// toggles
uniform int rainbowWater; // 1 = rainbow; 0 = default blue
uniform int partyMode;    // 1 = multiply boat albedo by moving rainbow tint

// Exposure & ambient
uniform float exposure;
uniform float ambientBoost;

// HSV helper
vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1., 2./3., 1./3., 3.);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6. - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0., 1.), c.y);
}

void main() {
    vec3 N = normalize(Normal);
    vec3 L = normalize(lightPos - FragPos);
    vec3 V = normalize(viewPos - FragPos);
    vec3 H = normalize(L + V);

    // Base color
    vec3 baseColor;
    vec2 uv = (invertV == 1) ? vec2(TexCoord.x, 1.0 - TexCoord.y) : TexCoord;

    if (isWater == 1) {
        if (rainbowWater == 1) {
            // smooth hue across XZ + time
            float hue = fract(0.15 * sin(FragPos.x*0.03 + time*0.7) + 0.15 * sin(FragPos.z*0.035 + time*1.1) + 0.5);
            baseColor = hsv2rgb(vec3(hue, 0.8, 0.9));
        } else {
            float w1 = sin(FragPos.x * 0.07 + time * 1.4);
            float w2 = sin(FragPos.z * 0.09 + time * 1.1);
            float w3 = sin((FragPos.x + FragPos.z) * 0.05 + time * 0.7);
            float shade = 0.5 + 0.5 * (0.5*w1 + 0.35*w2 + 0.15*w3);
            vec3 darkBlue  = vec3(0.05, 0.12, 0.28);
            vec3 lightBlue = vec3(0.18, 0.42, 0.78);
            baseColor = mix(darkBlue, lightBlue, shade);
        }
    } else if (useTexture == 1) {
        vec4 albedo = texture(texture_diffuse1, uv);
        if (albedo.a < 0.2) discard;
        baseColor = albedo.rgb;

        // Party tint only for boats when partyMode==1
        if (partyMode == 1) {
            float hue = fract(0.15 * sin(time*1.7) + 0.55);
            vec3 tint = hsv2rgb(vec3(hue, 0.8, 1.0));
            baseColor *= tint;
        }
    } else {
        baseColor = objectColor;
    }

    // Lighting
    float ambientK = ((isWater == 1) ? 0.24 : 0.22) * ambientBoost;
    vec3  ambient  = ambientK * baseColor * lightColor;

    float diff = max(dot(N, L), 0.0);
    vec3  diffuse = diff * baseColor * lightColor;

    float shininess = (isWater == 1) ? 160.0 : 32.0;
    float spec      = pow(max(dot(N, H), 0.0), shininess);
    float specK     = (isWater == 1) ? 1.6 : 0.3;
    vec3  specular  = specK * spec * lightColor;

    if (isWater == 1) {
        float sparkleMask = max(dot(N, L), 0.0);
        float sparkle = pow(sparkleMask, 16.0) * (0.25 + 0.25 * sin(time*3.0 + FragPos.x*0.2 + FragPos.z*0.17));
        specular += sparkle * lightColor;
    }

    if (useTexture == 1 && isWater == 0) {
        vec3 specTex = texture(texture_specular1, uv).rgb;
        specular *= specTex;
    }

    vec3 color = ambient + diffuse + specular;

    if (isWater == 1) {
        float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);
        color = mix(color, color + 0.22 * lightColor, 0.25 * fresnel);
    }

    color = vec3(1.0) - exp(-color * exposure);
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}
)";

    GLuint vs = compileShader(GL_VERTEX_SHADER,   kVertexSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, kFragmentSrc);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok = GL_FALSE; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len=0; glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetProgramInfoLog(prog, len, nullptr, log.data());
        std::cerr << "Shader linking failed: " << log << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

void Graphics::drawBoat(glm::vec3 position, float rotation, glm::vec3 scale, glm::vec3 color) {
    drawCube(position, rotation, scale, color);
}

void Graphics::drawCube(glm::vec3 position, float rotation, glm::vec3 scale, glm::vec3 color) {
    glBindVertexArray(cubeVAO);

    glm::mat4 m(1.0f);
    m = glm::translate(m, position);
    m = glm::rotate(m, glm::radians(rotation), glm::vec3(0,1,0));
    m = glm::scale(m, scale);

    GLint modelLoc   = glGetUniformLocation(shaderProgram, "model");
    GLint objColorLoc= glGetUniformLocation(shaderProgram, "objectColor");
    GLint useTexLoc  = glGetUniformLocation(shaderProgram, "useTexture");
    GLint isWaterLoc = glGetUniformLocation(shaderProgram, "isWater");
    GLint invertVLoc = glGetUniformLocation(shaderProgram, "invertV");
    GLint partyMode  = glGetUniformLocation(shaderProgram, "partyMode");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &m[0][0]);
    glUniform3fv(objColorLoc, 1, &color[0]);
    glUniform1i(useTexLoc, 0);
    glUniform1i(isWaterLoc, 0);
    glUniform1i(invertVLoc, 0);
    glUniform1i(partyMode,  0);

    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void Graphics::drawWater(const glm::vec3&, float, bool, bool) {
    glBindVertexArray(waterVAO);

    glm::mat4 m(1.0f);
    GLint modelLoc   = glGetUniformLocation(shaderProgram, "model");
    GLint useTexLoc  = glGetUniformLocation(shaderProgram, "useTexture");
    GLint isWaterLoc = glGetUniformLocation(shaderProgram, "isWater");
    GLint invertVLoc = glGetUniformLocation(shaderProgram, "invertV");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &m[0][0]);
    glUniform1i(useTexLoc, 0);
    glUniform1i(isWaterLoc, 1);
    glUniform1i(invertVLoc, 0);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Graphics::drawSkybox(const glm::vec3& playerPosition) {
    glBindVertexArray(skyboxVAO);

    glm::mat4 m(1.0f);
    m = glm::translate(m, playerPosition);
    m = glm::scale(m, glm::vec3(200.0f));

    GLint modelLoc   = glGetUniformLocation(shaderProgram, "model");
    GLint objColorLoc= glGetUniformLocation(shaderProgram, "objectColor");
    GLint useTexLoc  = glGetUniformLocation(shaderProgram, "useTexture");
    GLint isWaterLoc = glGetUniformLocation(shaderProgram, "isWater");
    GLint invertVLoc = glGetUniformLocation(shaderProgram, "invertV");
    GLint partyMode  = glGetUniformLocation(shaderProgram, "partyMode");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &m[0][0]);
    glUniform3f(objColorLoc, 0.65f, 0.85f, 1.0f);
    glUniform1i(useTexLoc, 0);
    glUniform1i(isWaterLoc, 0);
    glUniform1i(invertVLoc, 0);
    glUniform1i(partyMode,  0);

    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void Graphics::drawBoundaryWalls() { /* unused */ }

void Graphics::drawMountain(glm::vec3 position, glm::vec3 scale, glm::vec3 color, int textureIndex) {
    glBindVertexArray(mountainVAO);

    glm::mat4 m(1.0f);
    m = glm::translate(m, position);
    m = glm::scale(m, scale);

    GLint modelLoc   = glGetUniformLocation(shaderProgram, "model");
    GLint objColorLoc= glGetUniformLocation(shaderProgram, "objectColor");
    GLint useTexLoc  = glGetUniformLocation(shaderProgram, "useTexture");
    GLint isWaterLoc = glGetUniformLocation(shaderProgram, "isWater");
    GLint difLoc     = glGetUniformLocation(shaderProgram, "texture_diffuse1");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &m[0][0]);
    glUniform3fv(objColorLoc, 1, &color[0]);
    glUniform1i(isWaterLoc, 0);

    bool bound = false;
    if (textureIndex >= 0 && textureIndex < (int)mountainTextures.size()) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mountainTextures[textureIndex]);
        glUniform1i(difLoc, 0);
        bound = true;
    }
    glUniform1i(useTexLoc, bound ? 1 : 0);

    glDrawElements(GL_TRIANGLES, (GLsizei)mountainIndices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Graphics::drawXZCircle(const glm::vec3& center, float radius, const glm::vec3& color) {
    if (debugVAO == 0 || debugVBO == 0) return;

    const int segments = 64;
    std::vector<float> verts;
    verts.reserve(segments * 3);
    for (int i = 0; i < segments; ++i) {
        float t = (float)i / (float)segments;
        float ang = t * 2.0f * 3.14159265359f;
        float x = center.x + radius * std::cos(ang);
        float z = center.z + radius * std::sin(ang);
        verts.push_back(x);
        verts.push_back(center.y);
        verts.push_back(z);
    }

    glBindVertexArray(debugVAO);
    glBindBuffer(GL_ARRAY_BUFFER, debugVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_DYNAMIC_DRAW);

    glm::mat4 m(1.0f);
    GLint modelLoc   = glGetUniformLocation(shaderProgram, "model");
    GLint objColorLoc= glGetUniformLocation(shaderProgram, "objectColor");
    GLint useTexLoc  = glGetUniformLocation(shaderProgram, "useTexture");
    GLint isWaterLoc = glGetUniformLocation(shaderProgram, "isWater");
    GLint invertVLoc = glGetUniformLocation(shaderProgram, "invertV");
    GLint partyMode  = glGetUniformLocation(shaderProgram, "partyMode");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &m[0][0]);
    glUniform3fv(objColorLoc, 1, &color[0]);
    glUniform1i(useTexLoc, 0);
    glUniform1i(isWaterLoc, 0);
    glUniform1i(invertVLoc, 0);
    glUniform1i(partyMode,  0);

    glDrawArrays(GL_LINE_LOOP, 0, segments);
    glBindVertexArray(0);
}

void Graphics::generateSphere(float radius, int sectors, int stacks, std::vector<float>& vertices, std::vector<unsigned int>& indices) {
    const float PI = 3.14159265358979323846f;
    std::vector<glm::vec3> pos;
    std::vector<glm::vec3> norm;
    std::vector<glm::vec2> uv;

    for (int i = 0; i <= stacks; ++i) {
        float v = (float)i / (float)stacks;
        float phi = PI * v;
        float y = radius * std::cos(phi);
        float r = radius * std::sin(phi);

        for (int j = 0; j <= sectors; ++j) {
            float u = (float)j / (float)sectors;
            float theta = 2.0f * PI * u;
            float x = r * std::sin(theta);
            float z = r * std::cos(theta);

            glm::vec3 p(x, y, z);
            pos.push_back(p);
            norm.push_back(glm::normalize(p));
            uv.push_back(glm::vec2(u, v));
        }
    }

    vertices.clear();
    for (size_t i = 0; i < pos.size(); ++i) {
        vertices.insert(vertices.end(), { pos[i].x, pos[i].y, pos[i].z, norm[i].x, norm[i].y, norm[i].z, uv[i].x, uv[i].y });
    }

    indices.clear();
    const int stride = sectors + 1;
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectors; ++j) {
            int k1 = i * stride + j;
            int k2 = k1 + stride;

            indices.push_back(k1);
            indices.push_back(k2);
            indices.push_back(k1 + 1);

            indices.push_back(k1 + 1);
            indices.push_back(k2);
            indices.push_back(k2 + 1);
        }
    }
}

void Graphics::generateHalfSphere(float radius, int sectors, int stacks, std::vector<float>& vertices, std::vector<unsigned int>& indices) {
    const float PI = 3.14159265358979323846f;
    std::vector<glm::vec3> pos;
    std::vector<glm::vec3> norm;
    std::vector<glm::vec2> uv;

    for (int i = 0; i <= stacks; ++i) {
        float v   = (float)i / (float)stacks;
        float phi = (PI * 0.5f) * v; 
        float y   = radius * std::cos(phi);
        float r   = radius * std::sin(phi);

        for (int j = 0; j <= sectors; ++j) {
            float u = (float)j / (float)sectors;
            float theta = 2.0f * PI * u;
            float x = r * std::sin(theta);
            float z = r * std::cos(theta);

            glm::vec3 p(x, y, z);
            pos.push_back(p);
            norm.push_back(glm::normalize(p));
            uv.push_back(glm::vec2(u, v));
        }
    }

    vertices.clear();
    for (size_t i = 0; i < pos.size(); ++i) {
        vertices.insert(vertices.end(), { pos[i].x, pos[i].y, pos[i].z, norm[i].x, norm[i].y, norm[i].z, uv[i].x, uv[i].y });
    }

    indices.clear();
    const int stride = sectors + 1;
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectors; ++j) {
            int k1 = i * stride + j;
            int k2 = k1 + stride;

            indices.push_back(k1);
            indices.push_back(k2);
            indices.push_back(k1 + 1);

            indices.push_back(k1 + 1);
            indices.push_back(k2);
            indices.push_back(k2 + 1);
        }
    }
}










