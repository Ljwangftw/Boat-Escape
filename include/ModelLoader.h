#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include <vector>
#include <string>
#include <cstddef>   
#include <glm/glm.hpp>

#include <assimp/scene.h>
#include <assimp/material.h>

#include <glad/glad.h>


struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Texture {
    unsigned int id = 0;
    std::string  type;  
    std::string  path; 
};

class Mesh {
public:
    Mesh(const std::vector<Vertex>& verts,
         const std::vector<unsigned int>& inds,
         const std::vector<Texture>& texs);

    void Draw(unsigned int shaderProgram);

    const std::vector<Vertex>& getVertices() const { return vertices; }

private:
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture>      textures;

    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;

    void setupMesh();
};

class Model {
public:
    Model() = default;
    Model(const std::string& path, bool gamma = false);

    void Draw(unsigned int shaderProgram);

    glm::vec3 getBoundsMin() const;
    glm::vec3 getBoundsMax() const;

private:
    std::vector<Texture> textures_loaded; 
    std::vector<Mesh>    meshes;
    std::string          directory;

    void loadModel(const std::string& path);
    void processNode(struct aiNode* node, const struct aiScene* scene);
    Mesh processMesh(struct aiMesh* mesh, const struct aiScene* scene);

    std::vector<Texture> loadMaterialTextures(struct aiMaterial* mat,
                                              aiTextureType type,
                                              const std::string& typeName,
                                              const struct aiScene* scene);

    unsigned int loadTexture(const char* path, const struct aiScene* scene);
    static unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma=false);
};

#endif 
