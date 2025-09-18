#ifndef PROJECTILE_MANAGER_H
#define PROJECTILE_MANAGER_H

#include <vector>
#include <glm/glm.hpp>
#include <deque>

class MountainManager;
class ModelManager; 

struct EnhancedProjectile {
    glm::vec3 position;
    glm::vec3 velocity;
    bool active;
    float lifetime;
    bool isPlayerOwned;
    std::vector<glm::vec3> smokeTrail;
    float smokeTimer;
    
    EnhancedProjectile(glm::vec3 pos, glm::vec3 vel, bool playerOwned) 
        : position(pos), velocity(vel), active(true), lifetime(5.0f), 
          isPlayerOwned(playerOwned), smokeTimer(0.0f) {}
};

class ProjectileManager {
public:
    ProjectileManager();

    void Update(float dt, MountainManager& mountainManager);
    void AddProjectile(glm::vec3 pos, glm::vec3 vel, bool isPlayerOwned, bool clampToWater = true);
    void Clear();

    
    void DrawAll(unsigned int shader, ModelManager& modelManager);

    const std::vector<EnhancedProjectile>& GetProjectiles() const { return projectiles; }
    std::vector<EnhancedProjectile>& GetProjectiles() { return projectiles; }

private:
    std::vector<EnhancedProjectile> projectiles;
};

#endif
