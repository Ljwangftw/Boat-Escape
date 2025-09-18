#include "MountainManager.h"
#include <glm/gtc/constants.hpp>
#include <cstdlib>
#include <ctime>
#include <algorithm>


// Tunables / constants
const float SPAWN_RADIUS_MIN_MOUNTAIN = 90.0f;  
const float SPAWN_RADIUS_MAX_MOUNTAIN = 180.0f;  
const int   MAX_MOUNTAINS  = 6;      


static constexpr float kRespawnDistance = 380.0f;   
static constexpr float kRespawnAheadMin = 120.0f;    
static constexpr float kRespawnAheadMax = 200.0f;   
static constexpr float kWaterLevelY = -1.0f;
static constexpr float kScaleMin = 10.0f;
static constexpr float kScaleMax = 20.0f;
static constexpr float kMountainMeshBaseRadius = 3.0f;
static constexpr int kPlaceTries = 64;
static constexpr int kNumMountainTextures = 5;

// Helpers
static float frand01() {
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

static float frandRange(float a, float b) {
    return a + (b - a) * frand01();
}

static glm::vec2 randDir2() {
    float ang = frandRange(0.0f, 2.0f * glm::pi<float>());
    return glm::vec2(std::cos(ang), std::sin(ang));
}

static float dist2XZ(const glm::vec3& a, const glm::vec3& b) {
    float dx = a.x - b.x;
    float dz = a.z - b.z;
    return dx*dx + dz*dz;
}

// MountainManager
MountainManager::MountainManager()
    : spawnTimer(0.0f)
    , spawnInterval(0.5f) 
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
}

void MountainManager::Init() {
    mountains.clear();
    mountains.reserve(MAX_MOUNTAINS);
    spawnTimer = 0.0f;
}

void MountainManager::Update(float dt, const glm::vec3& playerPosition) {
    spawnTimer -= dt;
    while (static_cast<int>(mountains.size()) < MAX_MOUNTAINS && spawnTimer <= 0.0f) {
        SpawnMountain(playerPosition);
        spawnTimer += spawnInterval;
    }

    for (auto& m : mountains) {
        if (!m.active) continue;
        if (dist2XZ(m.position, playerPosition) > (kRespawnDistance * kRespawnDistance)) {
            RepositionFarMountain(m, playerPosition);
        }
    }
}

bool MountainManager::checkCollision(glm::vec3 p, float r) {
    for (const auto& m : mountains) {
        if (!m.active) continue;
        float rr = r + m.radius;
        if (dist2XZ(p, m.position) < rr * rr) {
            return true;
        }
    }
    return false;
}

// Spawning
void MountainManager::SpawnMountain(const glm::vec3& playerPosition) {
    Mountain m;
    m.active = true;
    m.textureIndex = std::min(kNumMountainTextures - 1, static_cast<int>(frand01() * kNumMountainTextures));

    float s = frandRange(kScaleMin, kScaleMax);
    m.scale  = glm::vec3(s);
    m.radius = kMountainMeshBaseRadius * s; 

    for (int t = 0; t < kPlaceTries; ++t) {
        float r   = frandRange(SPAWN_RADIUS_MIN_MOUNTAIN, SPAWN_RADIUS_MAX_MOUNTAIN);
        glm::vec2 dir = randDir2();
        glm::vec3 pos(playerPosition.x + dir.x * r,
                      kWaterLevelY,
                      playerPosition.z + dir.y * r);

        if (!checkCollision(pos, m.radius * 0.9f)) { 
            m.position = pos;
            mountains.push_back(m);
            return;
        }
    }
}

void MountainManager::RepositionFarMountain(Mountain& m, const glm::vec3& playerPosition) {
    m.textureIndex = std::min(kNumMountainTextures - 1, static_cast<int>(frand01() * kNumMountainTextures));
    float s = frandRange(kScaleMin, kScaleMax);
    m.scale  = glm::vec3(s);
    m.radius = kMountainMeshBaseRadius * s;


    for (int t = 0; t < kPlaceTries; ++t) {
        float r   = frandRange(kRespawnAheadMin, kRespawnAheadMax);
        glm::vec2 dir = randDir2();
        glm::vec3 pos(playerPosition.x + dir.x * r,
                      kWaterLevelY,
                      playerPosition.z + dir.y * r);

        if (!checkCollision(pos, m.radius * 0.9f)) {
            m.position = pos;
            return;
        }
    }
    // If no valid spot found, keep current position and try again next update.
}
