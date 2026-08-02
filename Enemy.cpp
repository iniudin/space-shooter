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
    for (int b = (int)bullets.size() - 1; b >= 0; b--) {
        for (int e = (int)enemies.size() - 1; e >= 0; e--) {
            Rectangle bulletRect = {
                bullets[b].position.x,
                bullets[b].position.y,
                bullets[b].size.x,
                bullets[b].size.y
            };

            Rectangle enemyRect = {
                enemies[e].position.x,
                enemies[e].position.y,
                enemies[e].size.x,
                enemies[e].size.y
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
    for (int i = (int)enemies.size() - 1; i >= 0; i--) {
        if (enemies[i].position.y > 450) {
            enemies.erase(enemies.begin() + i);
        }
    }
}

void DrawEnemies(std::vector<Enemy> &enemies) {
    for (const Enemy &e : enemies) {
        DrawRectangle(
            (int)e.position.x,
            (int)e.position.y,
            e.size.x,
            e.size.y,
            RED
        );
    }
}