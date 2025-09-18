#ifndef ENEMY_MANAGER_H
#define ENEMY_MANAGER_H

#include <vector>
#include <glm/glm.hpp>
#include "Game.h" 

class MountainManager;
class ProjectileManager;

struct EnemyBoat {
    glm::vec3 position;
    float rotation;
    float speed;
    bool active;
    float shootCooldown;
    float maxShootCooldown;
    
    EnemyBoat(glm::vec3 pos) : position(pos), rotation(0.0f), speed(2.0f), active(true), 
                               shootCooldown(0.0f), maxShootCooldown(2.0f) {}
};

class EnemyManager {
public:
    EnemyManager();

    void Init(Difficulty difficulty);
    void Update(float dt, const glm::vec3& playerPosition, ProjectileManager& projectileManager, MountainManager& mountainManager);
    void SpawnEnemy(const glm::vec3& playerPosition, MountainManager& mountainManager);
    
    const std::vector<EnemyBoat>& GetEnemies() const { return enemies; }
    std::vector<EnemyBoat>& GetEnemies() { return enemies; }

private:
    std::vector<EnemyBoat> enemies;
    float spawnTimer;
    Difficulty currentDifficulty;
    int maxEnemies;
};

#endif 
