#include "Bullet.h"
#include "raylib.h"

void UpdateBullets(std::vector<Bullet> &bullets) {
    float delta = GetFrameTime();

    // move all bullets upward
    for (Bullet &b : bullets) {
        b.position.y -= b.speed * delta;
    }

    // remove bullets that have gone off the top of the screen
    for (int i = (int)bullets.size() - 1; i >= 0; i--) {
        if (bullets[i].position.y < 0) {
            bullets.erase(bullets.begin() + i);
        }
    }
}

void DrawBullets(const std::vector<Bullet> &bullets) {
    for (const Bullet &b : bullets) {
        DrawRectangle((int)b.position.x, (int)b.position.y, 6, 16, YELLOW);
    }
}