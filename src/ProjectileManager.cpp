#include "ProjectileManager.h"
#include "MountainManager.h"
#include "ModelManager.h"
#include <iostream>

ProjectileManager::ProjectileManager() {}

void ProjectileManager::Update(float dt, MountainManager& mountainManager) {
    for (auto it = projectiles.begin(); it != projectiles.end();) {
        it->position += it->velocity * dt;
        it->lifetime -= dt;
        
        if (!it->isPlayerOwned) {
            it->smokeTimer += dt;
            if (it->smokeTimer >= 0.1f) {
                it->smokeTrail.push_back(it->position);
                it->smokeTimer = 0.0f;
                if (it->smokeTrail.size() > 8) {
                    it->smokeTrail.erase(it->smokeTrail.begin());
                }
            }
        }
        
        if (it->lifetime <= 0.0f || mountainManager.checkCollision(it->position, 0.1f)) {
            it = projectiles.erase(it);
        } else {
            ++it;
        }
    }
}

void ProjectileManager::AddProjectile(glm::vec3 pos, glm::vec3 vel, bool isPlayerOwned, bool clampToWater) {
    if (clampToWater) {
        const float WATER_LEVEL = -0.5f;
        pos.y = WATER_LEVEL + 0.05f;
    }
    projectiles.emplace_back(pos, vel, isPlayerOwned);
}

void ProjectileManager::Clear() {
    projectiles.clear();
}

void ProjectileManager::DrawAll(unsigned int shader, ModelManager& modelManager) {

    for (const auto& p : projectiles) {
        modelManager.DrawCannonball(shader, p.position, 1.0f);
    }
}

