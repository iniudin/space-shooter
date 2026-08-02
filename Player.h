#pragma once

#include "raylib.h"

struct Player {
    Vector2 position;
    float speed;
    Vector2 size;
};

void UpdatePlayer(Player &player, float screenWidth, float screenHeight);

void DrawPlayer(const Player &player, const Texture2D &texture, float frame);
