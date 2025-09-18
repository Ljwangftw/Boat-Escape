#include "ModelLoader.h"

#include <iostream>
#include <limits>
#include <algorithm>
#include <cstring>

// Assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "stb_image.h"

#ifndef aiTextureType_BASE_COLOR
#define aiTextureType_BASE_COLOR aiTextureType_DIFFUSE
#endif
#ifndef aiTextureType_NORMALS
#define aiTextureType_NORMALS aiTextureType_HEIGHT
#endif

Mesh::Mesh(const std::vector<Vertex>& verts,
           const std::vector<unsigned int>& inds,
           const std::vector<Texture>& texs)
    : vertices(verts), indices(inds), textures(texs) {
    setupMesh();
}

void Mesh::setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(Vertex),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);


    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, Position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, Normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, TexCoords));

    glBindVertexArray(0);
}

void Mesh::Draw(unsigned int shaderProgram) {

    unsigned int diffuseNr  = 1;
    unsigned int specularNr = 1;
    unsigned int normalNr   = 1;

    for (unsigned int i = 0; i < textures.size(); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);

        std::string name = textures[i].type;
        std::string number;
        if (name == "texture_diffuse")  number = std::to_string(diffuseNr++);
        if (name == "texture_specular") number = std::to_string(specularNr++);
        if (name == "texture_normal")   number = std::to_string(normalNr++);

        std::string uniformName = name + number;
        GLint loc = glGetUniformLocation(shaderProgram, uniformName.c_str());
        if (loc >= 0) glUniform1i(loc, i);

        glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()),
                   GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE0);
}


Model::Model(const std::string& path, bool /*gamma*/) {
    loadModel(path);
}

void Model::loadModel(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_SortByPType |
        aiProcess_GenUVCoords

    );

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        std::cerr << "Assimp error loading '" << path << "': "
                  << importer.GetErrorString() << std::endl;
        throw std::runtime_error("Assimp load failed");
    }

    size_t slash = path.find_last_of("/\\");
    directory = (slash == std::string::npos) ? std::string(".") : path.substr(0, slash);

    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene) {

    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture>      textures;

    vertices.reserve(mesh->mNumVertices);

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        Vertex v{};
        v.Position = glm::vec3(mesh->mVertices[i].x,
                               mesh->mVertices[i].y,
                               mesh->mVertices[i].z);

        if (mesh->HasNormals()) {
            v.Normal = glm::vec3(mesh->mNormals[i].x,
                                 mesh->mNormals[i].y,
                                 mesh->mNormals[i].z);
        } else {
            v.Normal = glm::vec3(0,1,0);
        }

        if (mesh->mTextureCoords[0] != nullptr) {
            v.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x,
                                    mesh->mTextureCoords[0][i].y);
        } else {
            v.TexCoords = glm::vec2(0.0f);
        }

        vertices.push_back(v);
    }


    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
            indices.push_back(face.mIndices[j]);
    }

    // Materials / textures
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        {
            auto diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

            auto baseColorMaps = loadMaterialTextures(material, aiTextureType_BASE_COLOR, "texture_diffuse", scene);
            textures.insert(textures.end(), baseColorMaps.begin(), baseColorMaps.end());
        }

        {
            auto specMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", scene);
            textures.insert(textures.end(), specMaps.begin(), specMaps.end());
        }

        {
            auto normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal", scene);
            textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        }
    }

    return Mesh(vertices, indices, textures);
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat,
                                                 aiTextureType type,
                                                 const std::string& typeName,
                                                 const aiScene* scene) {
    std::vector<Texture> textures;
    const unsigned int count = mat->GetTextureCount(type);
    for (unsigned int i = 0; i < count; ++i) {
        aiString str;
        if (mat->GetTexture(type, i, &str) != AI_SUCCESS) continue;


        bool skip = false;
        for (const auto& loaded : textures_loaded) {
            if (std::strcmp(loaded.path.c_str(), str.C_Str()) == 0) {
                textures.push_back(loaded);
                skip = true;
                break;
            }
        }
        if (!skip) {
            Texture tex;
            tex.id   = loadTexture(str.C_Str(), scene); 
            tex.type = typeName;
            tex.path = str.C_Str();
            textures.push_back(tex);
            textures_loaded.push_back(tex);
        }
    }
    return textures;
}


unsigned int Model::loadTexture(const char* path, const aiScene* scene) {
    unsigned int textureID = 0;
    glGenTextures(1, &textureID);

 
    stbi_set_flip_vertically_on_load(true);

    int width = 0, height = 0, channels = 0;
    unsigned char* data = nullptr;
    bool fromMemory = false;

    // Embedded texture
    if (scene && path && path[0] == '*') {
        const aiTexture* tex = scene->GetEmbeddedTexture(path);
        if (tex) {
            if (tex->mHeight == 0) {
                // Compressed (PNG/JPG) 
                data = stbi_load_from_memory(
                    reinterpret_cast<const unsigned char*>(tex->pcData),
                    static_cast<int>(tex->mWidth),
                    &width, &height, &channels, 0
                );
                fromMemory = true;
            } else {
                
                width  = static_cast<int>(tex->mWidth);
                height = static_cast<int>(tex->mHeight);
                channels = 4;

                std::vector<unsigned char> pixels(width * height * 4);
                for (int i = 0; i < width * height; ++i) {
                    pixels[i*4 + 0] = tex->pcData[i].r;
                    pixels[i*4 + 1] = tex->pcData[i].g;
                    pixels[i*4 + 2] = tex->pcData[i].b;
                    pixels[i*4 + 3] = tex->pcData[i].a;
                }

                glBindTexture(GL_TEXTURE_2D, textureID);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                             GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
                glGenerateMipmap(GL_TEXTURE_2D);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glBindTexture(GL_TEXTURE_2D, 0);
                return textureID;
            }
        }
    }

 
    if (!fromMemory) {
        std::string filename = std::string(path ? path : "");
        std::string full = directory.empty() ? filename : (directory + "/" + filename);
        data = stbi_load(full.c_str(), &width, &height, &channels, 0);
    }

    if (data) {
        GLenum format = (channels == 1) ? GL_RED : (channels == 3 ? GL_RGB : GL_RGBA);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, (GLint)format, width, height, 0,
                     format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(data);
    } else {
        std::cerr << "Warning: failed to load texture '" << (path ? path : "(null)") << "'\n";
        unsigned char white[4] = {255,255,255,255};
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, white);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    return textureID;
}

unsigned int Model::TextureFromFile(const char* path, const std::string& directory, bool /*gamma*/) {
    std::string filename = directory.empty() ? std::string(path) : (directory + "/" + path);

    stbi_set_flip_vertically_on_load(true);
    int w, h, ch;
    unsigned char* data = stbi_load(filename.c_str(), &w, &h, &ch, 0);

    unsigned int tex = 0;
    glGenTextures(1, &tex);

    if (data) {
        GLenum fmt = (ch == 1) ? GL_RED : (ch == 3 ? GL_RGB : GL_RGBA);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, (GLint)fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(data);
    } else {
        std::cerr << "TextureFromFile failed: " << filename << "\n";
    }
    return tex;
}

glm::vec3 Model::getBoundsMin() const {
    glm::vec3 mn( std::numeric_limits<float>::max());
    for (const auto& m : meshes) {
        for (const auto& v : m.getVertices()) {
            mn.x = std::min(mn.x, v.Position.x);
            mn.y = std::min(mn.y, v.Position.y);
            mn.z = std::min(mn.z, v.Position.z);
        }
    }
    return mn;
}

glm::vec3 Model::getBoundsMax() const {
    glm::vec3 mx(-std::numeric_limits<float>::max());
    for (const auto& m : meshes) {
        for (const auto& v : m.getVertices()) {
            mx.x = std::max(mx.x, v.Position.x);
            mx.y = std::max(mx.y, v.Position.y);
            mx.z = std::max(mx.z, v.Position.z);
        }
    }
    return mx;
}

void Model::Draw(unsigned int shaderProgram) {
    for (auto& m : meshes) {
        m.Draw(shaderProgram);
    }
}
