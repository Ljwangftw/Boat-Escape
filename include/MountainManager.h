#ifndef MOUNTAIN_MANAGER_H
#define MOUNTAIN_MANAGER_H

#include <vector>
#include <glm/glm.hpp>

extern const float SPAWN_RADIUS_MIN_MOUNTAIN;
extern const float SPAWN_RADIUS_MAX_MOUNTAIN;
extern const int   MAX_MOUNTAINS;

struct Mountain {
    glm::vec3 position;
    glm::vec3 scale;
    float radius;
    int   textureIndex;
    bool  active;
};

class MountainManager {
public:
    MountainManager();

    void Init();
    void Update(float dt, const glm::vec3& playerPosition);

  
    const std::vector<Mountain>& GetMountains() const { return mountains; }

    bool checkCollision(glm::vec3 position, float radius);

private:
    void SpawnMountain(const glm::vec3& playerPosition);
    void RepositionFarMountain(Mountain& m, const glm::vec3& playerPosition);

    std::vector<Mountain> mountains;
    float spawnTimer;
    float spawnInterval;
};

#endif 
