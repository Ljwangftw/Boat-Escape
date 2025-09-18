#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

#include <cstring>
#include "ModelLoader.h"
#include "BoatSkinIds.h"
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <assimp/material.h>
#include <memory>
#include <map>
#include <array>
#include <vector>
#include <string>

struct EulerOffset { float pitch, yaw, roll; };

struct GoingMerryTexture {
    unsigned int id = 0;
    std::string type;
    aiString path;
};

class ModelManager {
public:
    ModelManager();
    ~ModelManager();

    void LoadAllBoatModels();

    void DrawPlayerBoat(unsigned int shader,
                        int boatSkinIndex,
                        glm::vec3 position,
                        float rotation,
                        glm::vec3 scale);

    void DrawEnemyBoat(unsigned int shader,
                       glm::vec3 position,
                       float rotation,
                       glm::vec3 scale);

    void DrawCannonball(unsigned int shader,
                        glm::vec3 position,
                        float uniformScale = 1.0f);

    inline int  SkinCount() const { return static_cast<int>(boatModelPaths.size()); }
    inline bool ShouldFlipVForSkin(int idx) const {
        return (idx >= 0 && idx < static_cast<int>(playerFlipV.size())) ? playerFlipV[idx] : false;
    }
    inline bool ShouldFlipVEnemy() const { return enemyFlipV; }
    inline bool ShouldFlipVCannonball() const { return cannonballFlipV; }

private:
    struct NodeMesh {
        std::vector<Vertex>       vertices;
        std::vector<unsigned int> indices;
        std::vector<GoingMerryTexture> textures;
        glm::mat4                 transform;
        unsigned int              VAO = 0;
        unsigned int              VBO = 0;
        unsigned int              EBO = 0;

        void Setup();
        void Draw(unsigned int shader) const;
    };

    std::map<int, std::unique_ptr<Model>> playerBoats;
    std::map<int, float>                  playerLengthScale;
    std::vector<NodeMesh>                 goingMerryMeshes;
    std::vector<GoingMerryTexture>        goingMerryTexturesLoaded;

    std::unique_ptr<Model> enemyBoat;
    float                  enemyLengthScale = 1.0f;

    std::unique_ptr<Model> cannonball;
    float                  cannonballUnitScale = 1.0f;

    const std::vector<std::string> boatModelPaths = {
        "3D Model/thousand_sunny.glb",
        "3D Model/black_beard.glb",
        "3D Model/gol_d_roger.glb",
        "3D Model/buggy_clown.glb",
        "3D Model/big_mom.glb",
        "3D Model/going_merry.glb"
    };
    const std::vector<std::string> boatModelNames = {
        "Thousand Sunny",
        "Blackbeard Ship",
        "Gol D Roger Ship",
        "Buggy's Big Top",
        "Big Mom",
        "Going Merry"
    };

    const std::vector<float> modelScaleAdjustments = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    const std::vector<float> modelVerticalOffsets  = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    const float enemyScaleAdjustment = 1.0f;
    const float enemyVerticalOffset  = 0.0f;

    static constexpr float kBasePitchFix = -90.0f;

    static constexpr float kBigMomYawFixDeg = -2.0f;

    std::array<EulerOffset, BoatSkinId::COUNT> playerOrient = {
        EulerOffset{ 90.f, 360.f, 0.f },
        EulerOffset{ 0.f,  90.f,  90.f },
        EulerOffset{ 90.f, 180.f,  0.f },
        EulerOffset{ 0.f,  90.f,  90.f },
        EulerOffset{ 90.f,  90.f + kBigMomYawFixDeg,  0.f },
        EulerOffset{ 0.f,   0.f,  0.f }
    };

    EulerOffset enemyOrient = { 90.f, 270.f, 0.f };
    EulerOffset cannonballOrient = { 0.f, 0.f, 0.f };

    std::array<bool, BoatSkinId::COUNT> playerFlipV = { false, false, false, false, false, false };
    bool enemyFlipV                  = false;
    bool cannonballFlipV             = false;

    static float    computeXZLengthScale(const Model& model);
    static glm::mat4 makeOrient(const EulerOffset& off);

    void loadGoingMerryModel();
    void processGoingMerryNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform);
    NodeMesh processGoingMerryMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& transform);
    std::vector<GoingMerryTexture> loadGoingMerryMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName, const aiScene* scene);
    unsigned int loadGoingMerryTexture(const char* path, const aiScene* scene);
    static glm::mat4 aiMatrixToGlm(const aiMatrix4x4& from);
};

#endif 


