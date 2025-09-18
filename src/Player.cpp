#include "Player.h"
#include "ProjectileManager.h"
#include "MountainManager.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

Player::Player() { Reset(); }

void Player::Reset() {
    position     = glm::vec3(30.0f, -1.0f, 30.0f);
    velocity     = glm::vec3(0.0f);
    rotation     = 0.0f;
    shipFront    = glm::vec3(0.0f, 0.0f, 1.0f);

    baseSpeed    = 8.0f;
    baseBoost    = 16.0f;
    speed        = baseSpeed;
    boostSpeed   = baseBoost;

    maxHealth    = 300;
    health       = maxHealth;
    shootCooldown= 0.0f;
    gracePeriod  = 3.0f;
    boatSkinIndex= 0;
}

void Player::SetPhysicsMode(bool crazyOn) {
    if (crazyOn) {
        speed = baseSpeed * 1.75f;
        boostSpeed = baseBoost * 1.75f;
    } else {
        speed      = baseSpeed;
        boostSpeed = baseBoost;
    }
}

void Player::AlignToWater(float waterY) {
    if (boatSkinIndex == BoatSkinId::GOING_MERRY) {
        position.y = waterY + 0.25f;
    }
}

void Player::Update(float dt) {
    if (shootCooldown > 0.0f) shootCooldown -= dt;
    if (gracePeriod   > 0.0f) gracePeriod   -= dt;
}

void Player::ProcessGameInput(GLFWwindow* window, float dt, ProjectileManager& projectileManager, MountainManager& mountainManager) {
    const bool boosting = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                          glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    const float currentSpeed = boosting ? boostSpeed : speed;

    glm::vec3 proposed = position;

    glm::vec3 forward = glm::vec3(sin(glm::radians(rotation)), 0.0f, cos(glm::radians(rotation)));
    if (boatSkinIndex == BoatSkinId::GOING_MERRY) {
        forward = glm::vec3(cos(glm::radians(rotation)), 0.0f, -sin(glm::radians(rotation)));
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        proposed += forward * currentSpeed * dt;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        proposed -= forward * (currentSpeed * 0.7f * dt);
    }
    if (!mountainManager.checkCollision(proposed, 1.0f)) {
        position = proposed;
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) rotation += 90.0f * dt;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) rotation -= 90.0f * dt;

    const float shipYawRadians = glm::radians(rotation);
    if (boatSkinIndex == BoatSkinId::GOING_MERRY) {
        shipFront = glm::normalize(glm::vec3(cos(shipYawRadians), 0.0f, -sin(shipYawRadians)));
    } else {
        shipFront = glm::normalize(glm::vec3(sin(shipYawRadians), 0.0f, cos(shipYawRadians)));
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && shootCooldown <= 0.0f) {
        if (boatSkinIndex == BoatSkinId::GOING_MERRY) {
            glm::mat4 shipTransform = glm::mat4(1.0f);
            shipTransform = glm::translate(shipTransform, position);
            shipTransform = glm::rotate(shipTransform, shipYawRadians, glm::vec3(0.0f, 1.0f, 0.0f));
            shipTransform = glm::scale(shipTransform, glm::vec3(0.4f));
            glm::vec4 shipCenter = shipTransform * glm::vec4(1.0f, 0.0f, -0.875f, 1.0f);
            glm::vec3 actualShipCenter = glm::vec3(shipCenter);

            glm::vec3 modelUp = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 spawnPos = actualShipCenter + shipFront * 2.0f + modelUp * 0.2f;
            glm::vec3 vel      = shipFront * 20.0f;
            projectileManager.AddProjectile(spawnPos, vel, true, false);
            shootCooldown = 0.5f;
        } else {
            float yOffset = 0.0f;
            if (boatSkinIndex == BoatSkinId::BIG_MOM) yOffset = -0.5f;
            glm::vec3 muzzle = position + glm::vec3(sin(glm::radians(rotation)) * 2.0f, yOffset, cos(glm::radians(rotation)) * 2.0f);
            glm::vec3 vel    = glm::vec3(sin(glm::radians(rotation)) * 20.0f, 0.0f, cos(glm::radians(rotation)) * 20.0f);
            projectileManager.AddProjectile(muzzle, vel, true);
            shootCooldown = 0.1f;
        }
    }
}

void Player::TakeDamage(int damage) {
    if (gracePeriod > 0.0f) {
        std::cout << "Player is invincible! No damage taken.\n";
        return;
    }
    health -= damage;
    if (health < 0) health = 0;
    gracePeriod = 1.5f;
    std::cout << "Player took " << damage << " damage. Health: " << health << "\n";
}

void Player::AdjustRotation(float offset) {
    rotation += offset;
    if (rotation > 180.0f) rotation -= 360.0f;
    if (rotation < -180.0f) rotation += 360.0f;
}








