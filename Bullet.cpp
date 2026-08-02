#include "Bullet.h"
#include "raylib.h"

void UpdateBullets(std::vector<Bullet> &bullets) {
    const float delta = GetFrameTime();

    // move all bullets upward
    for (Bullet &b : bullets) {
        b.position.y -= b.speed * delta;
    }

    // remove bullets that have gone off the top of the screen
    for (int i = static_cast<int>(bullets.size()) - 1; i >= 0; i--) {
        if (bullets[i].position.y < 0) {
            bullets.erase(bullets.begin() + i);
        }
    }
}

void DrawBullets(const std::vector<Bullet> &bullets, const Texture2D &texture) {
    Rectangle source = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(texture.width),
        .height = static_cast<float>(texture.height),
    };
    constexpr Vector2 origin = {
        .x = 0.0f,
        .y = 0.0f,
    };
    for (const Bullet &b : bullets) {
        const Rectangle dest = {
            .x = b.position.x,
            .y = b.position.y,
            .width = b.size.x,
            .height = b.size.y,
        };

        DrawTexturePro(texture, source, dest, origin, 0.0f, WHITE);
    }
}
