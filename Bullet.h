#pragma once

#include "raylib.h"
#include <vector>

struct Bullet {
    Vector2 position;
    Vector2 size;
    float speed;
    int damage;
};

void UpdateBullets(std::vector<Bullet> &bullets);
void DrawBullets(const std::vector<Bullet> &bullets);