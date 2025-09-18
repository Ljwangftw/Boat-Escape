#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "Camera.h"
#include "ModelManager.h"
#include "EnemyManager.h"
#include "ProjectileManager.h"
#include "Player.h"
#include "MountainManager.h"

class Graphics {
public:
    Graphics();
    ~Graphics();

    void Init();

    void Render(const Player& player,
                const EnemyManager& enemyManager,
                const ProjectileManager& projectileManager,
                const MountainManager& mountainManager,
                float waveTime,
                float cameraYaw,
                float cameraPitch,
                float cameraDistance,
                float cameraHeight,
                bool  isFirstPerson,
                bool  enableRainbowWater,
                bool  enablePartyModeForPlayer,
                int   boatSkinIndex,
                bool  debugMountains,
                Camera& shipCamera,
                const glm::vec3& shipFront);

private:
    unsigned int shaderProgram;
    unsigned int goingMerryShader;

    // VAOs
    unsigned int shipVAO, waterVAO, skyboxVAO, cubeVAO;
    unsigned int mountainVAO, mountainVBO, mountainEBO;
    unsigned int sphereVAO, sphereVBO, sphereEBO;
    unsigned int debugVAO, debugVBO; // for simple line primitives

    std::unique_ptr<ModelManager> modelManager;

    std::vector<unsigned int> mountainTextures;
    std::vector<unsigned int> mountainIndices;

    void setupBuffers();
    void setupMountainBuffers();
    unsigned int loadTexture(const char* path);

    unsigned int compileShader(unsigned int type, const char* source);
    unsigned int createShaderProgram();

    void drawBoat(glm::vec3 position, float rotation, glm::vec3 scale, glm::vec3 color);
    void drawCube(glm::vec3 position, float rotation, glm::vec3 scale, glm::vec3 color);
    void drawWater(const glm::vec3& playerPosition, float waveTime, bool rainbow, bool party);
    void drawSkybox(const glm::vec3& playerPosition);
    void drawBoundaryWalls();
    void drawMountain(glm::vec3 position, glm::vec3 scale, glm::vec3 color, int textureIndex);

    void drawXZCircle(const glm::vec3& center, float radius, const glm::vec3& color);

    void generateSphere(float radius, int sectors, int stacks, std::vector<float>& vertices, std::vector<unsigned int>& indices);
    void generateHalfSphere(float radius, int sectors, int stacks, std::vector<float>& vertices, std::vector<unsigned int>& indices);
};

#endif // GRAPHICS_H

