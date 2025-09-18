#include "ModelManager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "stb_image.h"
#include <iostream>
#include <limits>
#include <algorithm>
#include <cstdlib>

ModelManager::ModelManager() {}
ModelManager::~ModelManager() {}

void ModelManager::NodeMesh::Setup() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

    glBindVertexArray(0);
}

void ModelManager::NodeMesh::Draw(unsigned int shader) const {
    const bool hasTexture = !textures.empty();
    GLint useTexLoc = glGetUniformLocation(shader, "useTexture");
    GLint useTexAltLoc = glGetUniformLocation(shader, "use_texture");
    if (useTexAltLoc >= 0) glUniform1i(useTexAltLoc, hasTexture ? 1 : 0);
    if (useTexLoc >= 0) glUniform1i(useTexLoc, hasTexture ? 1 : 0);

    unsigned int diffuseNr  = 1;
    unsigned int specularNr = 1;

    for (unsigned int i = 0; i < textures.size(); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        const auto& tex = textures[i];
        std::string number;
        if (tex.type == "texture_diffuse")  number = std::to_string(diffuseNr++);
        else if (tex.type == "texture_specular") number = std::to_string(specularNr++);
        std::string uniform = tex.type + number;
        GLint loc = glGetUniformLocation(shader, uniform.c_str());
        if (loc >= 0) glUniform1i(loc, i);
        glBindTexture(GL_TEXTURE_2D, tex.id);
    }

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
}

float ModelManager::computeXZLengthScale(const Model& model) {
    const glm::vec3 mn = model.getBoundsMin();
    const glm::vec3 mx = model.getBoundsMax();
    const float xExtent = mx.x - mn.x;
    const float zExtent = mx.z - mn.z;
    const float xzMax   = std::max(xExtent, zExtent);
    if (xzMax <= 1e-6f) return 1.0f;
    return 1.0f / xzMax;
}

glm::mat4 ModelManager::makeOrient(const EulerOffset& off) {
    glm::mat4 m(1.0f);
    m = glm::rotate(m, glm::radians(kBasePitchFix + off.pitch), glm::vec3(1,0,0));
    m = glm::rotate(m, glm::radians(off.yaw),  glm::vec3(0,1,0));
    m = glm::rotate(m, glm::radians(off.roll), glm::vec3(0,0,1));
    return m;
}

glm::mat4 ModelManager::aiMatrixToGlm(const aiMatrix4x4& from) {
    glm::mat4 to;
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}

void ModelManager::LoadAllBoatModels() {
    std::cout << "Loading boat models (" << boatModelPaths.size() << " skins)...\n";
    playerBoats.clear();
    playerLengthScale.clear();
    goingMerryMeshes.clear();
    goingMerryTexturesLoaded.clear();
    enemyBoat.reset();
    enemyLengthScale = 1.0f;
    cannonball.reset();
    cannonballUnitScale = 1.0f;

    for (int i = 0; i < static_cast<int>(boatModelPaths.size()); ++i) {
        const std::string& path = boatModelPaths[i];
        std::cout << "Loading: " << path << "\n";
        if (i == BoatSkinId::GOING_MERRY) {
            loadGoingMerryModel();
            if (goingMerryMeshes.empty()) {
                std::cerr << "Failed to load Going Merry node meshes.\n";
            } else {
                std::cout << "OK: " << boatModelNames[i] << " (node-based)\n";
            }
            continue;
        }
        try {
            auto mdl = std::make_unique<Model>(path);
            playerLengthScale[i] = computeXZLengthScale(*mdl);
            std::cout << "OK: " << boatModelNames[i]
                      << "  (XZ length scale = " << playerLengthScale[i] << ")\n";
            playerBoats[i] = std::move(mdl);
        } catch (const std::exception& e) {
            std::cerr << "Failed to load " << path << ": " << e.what() << "\n";
        }
    }

    try {
        std::cout << "Loading enemy model: 3D Model/marine_ship.glb\n";
        enemyBoat = std::make_unique<Model>("3D Model/marine_ship.glb");
        enemyLengthScale = computeXZLengthScale(*enemyBoat);
        std::cout << "Enemy model loaded. (XZ length scale = " << enemyLengthScale << ")\n";
    } catch (const std::exception& e) {
        std::cerr << "Enemy model failed: " << e.what() << "\n";
        if (playerBoats.count(0) && playerBoats.at(0)) {
            enemyBoat.reset();
            enemyLengthScale = playerLengthScale.count(0) ? playerLengthScale[0] : 1.0f;
            std::cerr << "Enemy will fallback to player model 0.\n";
        }
    }

    try {
        std::cout << "Loading projectile model: 3D Model/cannonball.glb\n";
        cannonball = std::make_unique<Model>("3D Model/cannonball.glb");
        cannonballUnitScale = computeXZLengthScale(*cannonball);
        std::cout << "Cannonball model loaded. (XZ length scale = "
                  << cannonballUnitScale << ")\n";
    } catch (const std::exception& e) {
        std::cerr << "Cannonball model failed: " << e.what() << "\n";
        cannonball.reset();
        cannonballUnitScale = 1.0f;
    }

    std::cout << "Model loading done. Player skins: " << playerBoats.size()
              << "  Enemy: " << (enemyBoat ? "OK" : "FALLBACK")
              << "  Cannonball: " << (cannonball ? "OK" : "MISSING")
              << "  Going Merry meshes: " << goingMerryMeshes.size()
              << "\n";
}

void ModelManager::DrawPlayerBoat(unsigned int shader,
                                  int boatSkinIndex,
                                  glm::vec3 position,
                                  float rotation,
                                  glm::vec3 scale) {
    if (boatSkinIndex == BoatSkinId::GOING_MERRY) {
        if (goingMerryMeshes.empty()) {
            loadGoingMerryModel();
            if (goingMerryMeshes.empty()) return;
        }
        glm::mat4 modelBase(1.0f);
        modelBase = glm::translate(modelBase, position);
        modelBase = glm::rotate(modelBase, glm::radians(rotation), glm::vec3(0,1,0));
        modelBase = glm::scale(modelBase, glm::vec3(0.4f));

        GLint modelLoc = glGetUniformLocation(shader, "model");
        for (const auto& mesh : goingMerryMeshes) {
            glm::mat4 model = modelBase * mesh.transform;
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            mesh.Draw(shader);
        }
        return;
    }

    if (boatSkinIndex < 0 || boatSkinIndex >= static_cast<int>(boatModelPaths.size()))
        boatSkinIndex = 0;

    Model* mdl = nullptr;
    if (auto it = playerBoats.find(boatSkinIndex); it != playerBoats.end())
        mdl = it->second.get();
    if (!mdl) {
        auto it0 = playerBoats.find(0);
        if (it0 == playerBoats.end() || !it0->second) return;
        mdl = it0->second.get();
        boatSkinIndex = 0;
    }

    const float lenScale = playerLengthScale.count(boatSkinIndex) ? playerLengthScale[boatSkinIndex] : 1.0f;
    const float sAdj     = modelScaleAdjustments[boatSkinIndex];

    glm::vec3 adjustedScale = scale * lenScale * sAdj;
    glm::vec3 adjustedPos   = position;
    adjustedPos.y += modelVerticalOffsets[boatSkinIndex];

    glm::mat4 T = glm::translate(glm::mat4(1.0f), adjustedPos);
    glm::mat4 Y = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0,1,0));
    glm::mat4 O = makeOrient(playerOrient[boatSkinIndex]);
    glm::mat4 S = glm::scale(glm::mat4(1.0f), adjustedScale);
    glm::mat4 M = T * Y * O * S;

    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &M[0][0]);
    mdl->Draw(shader);
}

void ModelManager::DrawEnemyBoat(unsigned int shader,
                                 glm::vec3 position,
                                 float rotation,
                                 glm::vec3 scale) {
    Model* mdl = enemyBoat ? enemyBoat.get()
                           : (playerBoats.count(0) ? playerBoats.at(0).get() : nullptr);
    if (!mdl) { std::cerr << "No enemy model available.\n"; return; }

    const float lenScale = enemyBoat ? enemyLengthScale
                                     : (playerLengthScale.count(0) ? playerLengthScale[0] : 1.0f);

    glm::vec3 adjustedScale = scale * lenScale * enemyScaleAdjustment;
    glm::vec3 adjustedPos   = position;
    adjustedPos.y += enemyVerticalOffset;

    glm::mat4 T = glm::translate(glm::mat4(1.0f), adjustedPos);
    glm::mat4 Y = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0,1,0));
    glm::mat4 O = makeOrient(enemyOrient);
    glm::mat4 S = glm::scale(glm::mat4(1.0f), adjustedScale);
    glm::mat4 M = T * Y * O * S;

    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &M[0][0]);
    mdl->Draw(shader);
}

void ModelManager::DrawCannonball(unsigned int shader,
                                  glm::vec3 position,
                                  float uniformScale) {
    if (!cannonball) return;

    const float sizeMultiplier = 3.5f;

    glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 S = glm::scale(glm::mat4(1.0f),
                             glm::vec3(cannonballUnitScale * uniformScale * sizeMultiplier));
    glm::mat4 M = T * S;

    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &M[0][0]);
    cannonball->Draw(shader);
}

void ModelManager::loadGoingMerryModel() {
    goingMerryMeshes.clear();
    goingMerryTexturesLoaded.clear();

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile("3D Model/going_merry.glb",
        aiProcess_Triangulate |
        aiProcess_FlipUVs);

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << "\n";
        return;
    }

    processGoingMerryNode(scene->mRootNode, scene, glm::mat4(1.0f));
}

void ModelManager::processGoingMerryNode(aiNode* node,
                                         const aiScene* scene,
                                         const glm::mat4& parentTransform) {
    glm::mat4 transform = parentTransform * aiMatrixToGlm(node->mTransformation);
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        goingMerryMeshes.push_back(processGoingMerryMesh(mesh, scene, transform));
    }
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        processGoingMerryNode(node->mChildren[i], scene, transform);
    }
}

ModelManager::NodeMesh ModelManager::processGoingMerryMesh(aiMesh* mesh,
                                                           const aiScene* scene,
                                                           const glm::mat4& transform) {
    NodeMesh gmMesh;
    gmMesh.transform = transform;

    gmMesh.vertices.reserve(mesh->mNumVertices);
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        Vertex vertex{};
        vertex.Position = glm::vec3(mesh->mVertices[i].x,
                                    mesh->mVertices[i].y,
                                    mesh->mVertices[i].z);
        if (mesh->HasNormals()) {
            vertex.Normal = glm::vec3(mesh->mNormals[i].x,
                                      mesh->mNormals[i].y,
                                      mesh->mNormals[i].z);
        } else {
            vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        if (mesh->mTextureCoords[0]) {
            vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x,
                                         mesh->mTextureCoords[0][i].y);
        } else {
            vertex.TexCoords = glm::vec2(0.0f);
        }
        gmMesh.vertices.push_back(vertex);
    }

    gmMesh.indices.reserve(mesh->mNumFaces * 3);
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            gmMesh.indices.push_back(face.mIndices[j]);
        }
    }

    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    auto diffuseMaps = loadGoingMerryMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
    gmMesh.textures.insert(gmMesh.textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    auto specularMaps = loadGoingMerryMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", scene);
    gmMesh.textures.insert(gmMesh.textures.end(), specularMaps.begin(), specularMaps.end());

    gmMesh.Setup();
    return gmMesh;
}

std::vector<GoingMerryTexture> ModelManager::loadGoingMerryMaterialTextures(aiMaterial* mat,
                                                                            aiTextureType type,
                                                                            const std::string& typeName,
                                                                            const aiScene* scene) {
    std::vector<GoingMerryTexture> textures;
    if (!mat) return textures;

    for (unsigned int i = 0; i < mat->GetTextureCount(type); ++i) {
        aiString str;
        mat->GetTexture(type, i, &str);
        bool skip = false;
        for (const auto& loaded : goingMerryTexturesLoaded) {
            if (std::strcmp(loaded.path.C_Str(), str.C_Str()) == 0) {
                textures.push_back(loaded);
                skip = true;
                break;
            }
        }
        if (!skip) {
            GoingMerryTexture texture;
            texture.id = loadGoingMerryTexture(str.C_Str(), scene);
            texture.type = typeName;
            texture.path = str;
            textures.push_back(texture);
            goingMerryTexturesLoaded.push_back(texture);
        }
    }
    return textures;
}

unsigned int ModelManager::loadGoingMerryTexture(const char* path, const aiScene* scene) {
    stbi_set_flip_vertically_on_load(false);
    std::string filename = path ? std::string(path) : std::string();
    unsigned int textureID = 0;
    glGenTextures(1, &textureID);

    int width = 0, height = 0, nrComponents = 0;
    unsigned char* data = nullptr;

    if (!filename.empty() && filename[0] == '*') {
        int textureIndex = std::atoi(filename.c_str() + 1);
        if (scene && textureIndex >= 0 && textureIndex < static_cast<int>(scene->mNumTextures)) {
            aiTexture* embedded = scene->mTextures[textureIndex];
            if (embedded->mHeight == 0) {
                data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(embedded->pcData),
                                             embedded->mWidth,
                                             &width, &height, &nrComponents, 0);
            } else {
                width  = static_cast<int>(embedded->mWidth);
                height = static_cast<int>(embedded->mHeight);
                nrComponents = 4;
                std::vector<unsigned char> pixels(width * height * 4);
                for (int i = 0; i < width * height; ++i) {
                    pixels[i*4 + 0] = embedded->pcData[i].r;
                    pixels[i*4 + 1] = embedded->pcData[i].g;
                    pixels[i*4 + 2] = embedded->pcData[i].b;
                    pixels[i*4 + 3] = embedded->pcData[i].a;
                }
                glBindTexture(GL_TEXTURE_2D, textureID);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
                glGenerateMipmap(GL_TEXTURE_2D);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glBindTexture(GL_TEXTURE_2D, 0);
                return textureID;
            }
        }
    } else {
        data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    }

    if (data) {
        GLenum format = GL_RGB;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(data);
    } else {
        std::cerr << "Texture failed to load: " << filename << "\n";
        unsigned char white[4] = {255,255,255,255};
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    return textureID;
}




