#include "Enemy.h"
#include "raylib.h"
#include <vector>
void UpdateEnemies(std::vector<Enemy> &enemies, std::vector<Bullet> &bullets) {
    float delta = GetFrameTime();

    // move all enemies downward
    for (Enemy &e : enemies) {
        e.position.y += e.speed * delta;
    }

    // check collision between each bullet and each enemy
    for (int b = static_cast<int>(bullets.size()) - 1; b >= 0; b--) {
        for (int e = static_cast<int>(enemies.size()) - 1; e >= 0; e--) {
            Rectangle bulletRect = {
                .x = bullets[b].position.x,
                .y = bullets[b].position.y,
                .width = bullets[b].size.x,
                .height = bullets[b].size.y
            };

            Rectangle enemyRect = {
                .x = enemies[e].position.x,
                .y = enemies[e].position.y,
                .width = enemies[e].size.x,
                .height = enemies[e].size.y
            };

            if (CheckCollisionRecs(bulletRect, enemyRect)) {
                enemies[e].health -= bullets[b].damage;
                bullets.erase(bullets.begin() + b);

                if (enemies[e].health <= 0) {
                    enemies.erase(enemies.begin() + e);
                }

                break;
            }
        }
    }

    // remove enemies that have gone off the bottom of the screen
    for (int i = static_cast<int>(enemies.size()) - 1; i >= 0; i--) {
        if (enemies[i].position.y > 450) {
            enemies.erase(enemies.begin() + i);
        }
    }
}

void DrawEnemies(const std::vector<Enemy> &enemies, const Texture2D &texture) {
    constexpr float frameCount = 4.0f;

    const float frameWidth = static_cast<float>(texture.width) / frameCount;
    const auto frameHeight = static_cast<float>(texture.height);

    // all enemies share the same animation clock for now
    const int frame = static_cast<int>(GetTime() / 0.1f) % 4;

    Rectangle source = {
        .x = static_cast<float>(frame) * frameWidth,
        .y = frameHeight,
        .width = frameWidth,
        .height = -frameHeight,
    };

    constexpr Vector2 origin = {.x = 0.0f, .y = 0.0f};
    for (const Enemy &e : enemies) {
        constexpr float frameMultiple = 2.0f;
        const Rectangle dest = {
            .x = e.position.x,
            .y = e.position.y,
            .width = frameWidth * frameMultiple,
            .height = frameHeight * frameMultiple,
        };
        DrawTexturePro(texture, source, dest, origin, 0.0f, WHITE);
    }
}
