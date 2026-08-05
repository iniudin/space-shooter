#pragma once
#include <raylib.h>
#include <vector>

#include "Entity.h"
#include "Player.h"


class Game {
public:
    Game();
    ~Game();
    void Run();

private:
    void SpawnEnemies(float deltaTime);
    void FireBullets(float deltaTime);
    void ResolveCollisions() const;
    void RemoveDead();

    Texture2D playerTexture{};
    Texture2D enemyTexture{};
    Texture2D bulletTexture{};
    Texture2D bgTexture{};

    std::vector<std::unique_ptr<Entity>> things;
    Player *player = nullptr;

    float spawnTimer = 0.0f;
    float spawnInterval = 2.0f;
    float fireTimer = 0.0f;
    float gameTime = 0.0f;
};
