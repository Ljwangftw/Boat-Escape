#include "EnemyManager.h"
#include "ProjectileManager.h"
#include "MountainManager.h"
#include <random>
#include <iostream>
#include <glm/gtc/constants.hpp>
#include <algorithm>

static const float SPAWN_RADIUS_MIN = 60.0f;
static const float SPAWN_RADIUS_MAX = 100.0f;

EnemyManager::EnemyManager() : spawnTimer(0.0f), currentDifficulty(EASY), maxEnemies(30) {}

void EnemyManager::Init(Difficulty difficulty) {
    currentDifficulty = difficulty;
    enemies.clear();
    maxEnemies = (currentDifficulty == EASY) ? 100 : 200;
    spawnTimer = -3.0f; 
}

void EnemyManager::Update(float dt, const glm::vec3& playerPosition, ProjectileManager& projectileManager, MountainManager& mountainManager) {
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
        [&](const EnemyBoat& e){ return glm::length(e.position - playerPosition) > SPAWN_RADIUS_MAX + 20.0f; }),
        enemies.end());

    // Spawn
    spawnTimer += dt;
    if (spawnTimer >= 1.0f && enemies.size() < static_cast<size_t>(maxEnemies)) {
        const int toSpawn = (currentDifficulty == EASY) ? 10 : 20;
        for (int i = 0; i < toSpawn && enemies.size() < static_cast<size_t>(maxEnemies); ++i) {
            SpawnEnemy(playerPosition, mountainManager);
        }
        spawnTimer = 0.0f;
    }

    // Think / move / shoot
    for (auto& enemy : enemies) {
        if (!enemy.active) continue;

        if (enemy.shootCooldown > 0.0f) enemy.shootCooldown -= dt;

        glm::vec3 dir = playerPosition - enemy.position;
        const float dist = glm::length(dir);
        const float minPlayerDistance = 5.0f;

        glm::vec3 oldPos = enemy.position;
        if (dist > minPlayerDistance + 3.0f)      enemy.position += glm::normalize(dir) * enemy.speed * dt;
        else if (dist < minPlayerDistance)        enemy.position -= glm::normalize(dir) * enemy.speed * 0.5f * dt;

        if (mountainManager.checkCollision(enemy.position, 1.0f)) enemy.position = oldPos;

        if (dist > 0.1f) enemy.rotation = atan2(dir.x, dir.z) * 180.0f / glm::pi<float>();

        if (dist <= 25.0f && dist > minPlayerDistance && enemy.shootCooldown <= 0.0f) {
            glm::vec3 muzzle = enemy.position + glm::vec3(sin(glm::radians(enemy.rotation)) * 2.0f, 0.0f, cos(glm::radians(enemy.rotation)) * 2.0f);
            glm::vec3 vel    = glm::normalize(dir) * 8.0f;
            projectileManager.AddProjectile(muzzle, vel, false);
            enemy.shootCooldown = enemy.maxShootCooldown;
        }
    }
}

void EnemyManager::SpawnEnemy(const glm::vec3& playerPosition, MountainManager& mountainManager) {
    std::random_device rd; std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angle(0.0f, 2.0f * glm::pi<float>());
    std::uniform_real_distribution<float> radius(SPAWN_RADIUS_MIN, SPAWN_RADIUS_MAX);

    glm::vec3 pos;
    bool ok = false;
    int attempts = 0;
    do {
        const float a = angle(gen);
        const float r = radius(gen);
        pos = playerPosition + glm::vec3(cos(a) * r, -1.0f, sin(a) * r); 
        ok = !mountainManager.checkCollision(pos, 1.0f);
        if (ok) {
            for (const auto& e : enemies) {
                if (e.active && glm::length(pos - e.position) < 5.0f) { ok = false; break; }
            }
        }
        attempts++;
    } while (!ok && attempts < 50);

    if (ok) enemies.emplace_back(pos);
}
