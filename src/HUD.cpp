#include "HUD.h"
#include "raylib.h"

void DrawSpeed(const int speed) {
    DrawText(TextFormat("Speed %d", speed), 10, 10, 30, LIGHTGRAY);
};

void DrawBulletCount(const int count) {
    DrawText(TextFormat("Bullet: %d", count), 10, 50, 30, LIGHTGRAY);
};
