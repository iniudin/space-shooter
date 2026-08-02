#include "Bullet.h"
#include "Enemy.h"
#include "HUD.h"
#include "Player.h"
#include "raylib.h"
#include <vector>

int main() {

    const int screenWidth = 800;  // x
    const int screenHeight = 450; // y

    InitWindow(screenWidth, screenHeight, "Pesawat Tempur");
    SetTargetFPS(144);

    float gameTime = 0.0f;

    Player player;
    player.position = {400, 400};
    player.size = 40;
    player.speed = 200.0f;

    float fireTimer = 0.0f;
    std::vector<Bullet> bullets;

    float spawnTimer = 0.0f;
    float spawnInterval = 2.f;
    std::vector<Enemy> enemies;

    while (!WindowShouldClose()) {
        spawnTimer += GetFrameTime();
        if (spawnTimer >= spawnInterval) {
            gameTime = gameTime * 5.f;

            Enemy e;
            e.position.x = (float)GetRandomValue(0, 760); // random x on screen
            e.position.y = -30; // just abouve the top edge
            e.speed = (float)(GetRandomValue(
                80,
                (int)(200 + gameTime * 5.0f)
            )); // random speed
            e.health = GetRandomValue(3, 10);
            e.size = {20, 20};
            enemies.push_back(e);
            spawnTimer = 0.0f;
        }

        UpdateEnemies(enemies, bullets);

        UpdatePlayer(player, screenWidth, screenHeight);

        fireTimer += GetFrameTime();
        if (IsKeyDown(KEY_X) && fireTimer >= 0.15f) {
            Bullet b;
            b.position.x = player.position.x + player.size / 2.0f - 3;
            b.position.y = player.position.y;
            b.speed = 500.0f;
            b.size = {6, 10};
            bullets.push_back(b);
            fireTimer = 0.f;
        }
        UpdateBullets(bullets);

        BeginDrawing();
        ClearBackground(DARKGRAY);
        DrawEnemies(enemies);
        DrawPlayer(player);
        DrawBullets(bullets);
        DrawBulletCount((int)bullets.size());
        DrawSpeed((int)player.speed);
        DrawFPS(750, 10);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}