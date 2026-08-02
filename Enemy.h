#pragma once

#include "Bullet.h"
#include "raylib.h"
#include <vector>

struct Enemy {
    Vector2 position;
    Vector2 size;
    float speed;
    int health;
};

void UpdateEnemies(std::vector<Enemy> &enemies, std::vector<Bullet> &bullets);
void DrawEnemies(std::vector<Enemy> &enemies);