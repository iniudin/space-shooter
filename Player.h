#pragma once

#include "raylib.h"

struct Player {
    Vector2 position;
    float speed;
    int size;
};

void UpdatePlayer(Player &player, int screenWidth, int screenHeight);

void DrawPlayer(Player player);