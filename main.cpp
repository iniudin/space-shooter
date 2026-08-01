#include "Bullet.h"
#include "HUD.h"
#include "Player.h"
#include "raylib.h"
#include <vector>

int main() {

    const int screenWidth = 800;  // x
    const int screenHeight = 450; // y

    InitWindow(screenWidth, screenHeight, "Pesawat Tempur");
    SetTargetFPS(144);

    Player player;
    player.position = {400, 400};
    player.size = 40;
    player.speed = 200.0f;

    float fireTimer = 0.0f;
    std::vector<Bullet> bullets;

    while (!WindowShouldClose()) {
        UpdatePlayer(player, screenWidth, screenHeight);
        UpdateBullets(bullets);

        fireTimer += GetFrameTime();
        if (IsKeyDown(KEY_X) && fireTimer >= 0.15f) {
            Bullet b;
            b.position.x = player.position.x + player.size / 2.0f - 3;
            b.position.y = player.position.y;
            b.speed = 500.0f;
            bullets.push_back(b);
            fireTimer = 0.f;
        }

        BeginDrawing();
        ClearBackground(DARKGRAY);
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