#ifndef PLAYER_H
#define PLAYER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "BoatSkinIds.h"

class ProjectileManager;
class MountainManager;

class Player {
public:
    Player();

    void Update(float dt);
    void ProcessGameInput(GLFWwindow* window, float dt, ProjectileManager& projectileManager, MountainManager& mountainManager);
    void Reset();
    void TakeDamage(int damage);

    glm::vec3 GetPosition() const { return position; }
    float     GetRotation() const { return rotation; }
    int       GetHealth()   const { return health; }
    int       GetMaxHealth()const { return maxHealth; }
    const glm::vec3& GetShipFront() const { return shipFront; }

    void SetPosition(const glm::vec3& pos) { position = pos; }
    void SetRotation(float rot) { rotation = rot; }
    void SetHealth(int hp) { health = hp; }
    void AdjustRotation(float offset);

    void SetPhysicsMode(bool crazyOn);
    void SetBoatSkin(int idx) { boatSkinIndex = idx; }
    void AlignToWater(float waterY);

private:
    glm::vec3 position;
    glm::vec3 velocity;
    float rotation;
    glm::vec3 shipFront;

    float baseSpeed;
    float baseBoost;

    float speed;
    float boostSpeed;

    int   health;
    int   maxHealth;
    float shootCooldown;
    float gracePeriod;

    // Current player boat skin index (0..5). 5 = Going Merry.
    int   boatSkinIndex = 0;
};

#endif 
