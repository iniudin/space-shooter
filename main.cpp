#include "HUD.h"
#include "Player.h"
#include "raylib.h"

int main() {

    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Pesawat Tempur");
    SetTargetFPS(90);

    Player player;
    player.position = {400, 200};
    player.size = 40;
    player.speed = 200.0f;

    while (!WindowShouldClose()) {
        UpdatePlayer(player, screenWidth, screenHeight);

        BeginDrawing();
        ClearBackground(DARKGRAY);
        DrawPlayer(player);
        DrawSpeed(player);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}