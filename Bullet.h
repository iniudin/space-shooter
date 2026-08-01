#pragma once

#include "raylib.h"
#include <vector>

struct Bullet {
    Vector2 position;
    float speed;
};

void UpdateBullets(std::vector<Bullet> &bullets);
void DrawBullets(const std::vector<Bullet> &bullets);