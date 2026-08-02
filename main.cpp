#include "Bullet.h"
#include "Enemy.h"
#include "HUD.h"
#include "Player.h"
#include "Sprite.h"
#include "raylib.h"
#include <vector>

int main() {

    constexpr float SCREEN_WIDTH = 800.0f;
    constexpr float SCREEN_HEIGHT = 450.0f;
    constexpr float SPRITE_SCALE = 2.0f;
    constexpr float FRAME_COUNT = 4.0f;

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Space Shooter");
    SetTargetFPS(144);

    Texture2D playerTexture =
        LoadTexture("assets/spaceships/spr_spaceship_01_animation.png");
    Texture2D enemyTexture = LoadTexture(
        "assets/enemy_spaceships/spr_enemy_spaceship_01_animation.png"
    );
    Texture2D bulletTexture =
        LoadTexture("assets/spaceships/bullets/spr_spaceship_bullet_02.png");
    Texture2D bgTexture =
        LoadTexture("assets/backgrounds/spr_background_01.png");

    if (playerTexture.id == 0) {
        TraceLog(
            LOG_WARNING,
            "Could not load player texture - check the path!"
        );
    }

    float gameTime = 0.0f;

    float animTimer = 0.0f;

    Player player{};
    player.position = {.x = 400, .y = 400};
    player.size = GetFrameSize(playerTexture, FRAME_COUNT, SPRITE_SCALE);
    player.speed = 200.0f;

    float fireTimer = 0.0f;
    std::vector<Bullet> bullets;

    float spawnTimer = 0.0f;
    float spawnInterval = 2.f;
    std::vector<Enemy> enemies;

    while (!WindowShouldClose()) {
        constexpr float frameTime = 0.1f;
        animTimer += GetFrameTime();
        const int playerFrame = static_cast<int>(animTimer / frameTime) % 4;

        spawnTimer += GetFrameTime();
        if (spawnTimer >= spawnInterval) {
            gameTime = gameTime * 5.f;

            Enemy e{};
            e.position.x = static_cast<float>(GetRandomValue(0, 760));
            e.position.y = -30;
            e.speed = static_cast<float>(
                GetRandomValue(80, static_cast<int>(200 + gameTime * 5.0f))
            );
            e.health = GetRandomValue(2, 8);
            e.size = GetFrameSize(enemyTexture, FRAME_COUNT, SPRITE_SCALE);
            enemies.push_back(e);
            spawnTimer = 0.0f;
        }

        UpdateEnemies(enemies, bullets);

        UpdatePlayer(player, SCREEN_WIDTH, SCREEN_HEIGHT);

        fireTimer += GetFrameTime();
        if (IsKeyDown(KEY_X) && fireTimer >= 0.15f) {
            Bullet b{};
            b.position.x = player.position.x + player.size.x / 2.0f - 3;
            b.position.y = player.position.y;
            b.speed = 500.0f;
            b.damage = 1;
            b.size = GetFrameSize(bulletTexture, FRAME_COUNT, SPRITE_SCALE);
            bullets.push_back(b);
            fireTimer = 0.f;
        }
        UpdateBullets(bullets);

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(
            bgTexture,
            {.x = 0, .y = 0, .width = 256, .height = 256},
            {.x = 0, .y = 0, .width = SCREEN_WIDTH, .height = SCREEN_HEIGHT},
            {.x = 0, .y = 0},
            0.0f,
            WHITE
        );
        DrawEnemies(enemies, enemyTexture);
        DrawPlayer(player, playerTexture, static_cast<float>(playerFrame));
        DrawBullets(bullets, bulletTexture);
        DrawBulletCount(static_cast<int>(bullets.size()));
        DrawSpeed(static_cast<int>(player.speed));
        DrawFPS(750, 10);
        EndDrawing();
    }

    UnloadTexture(playerTexture);
    UnloadTexture(enemyTexture);
    UnloadTexture(bulletTexture);
    UnloadTexture(bgTexture);

    CloseWindow();
    return 0;
}
