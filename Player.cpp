#include "Player.h"
#include "raylib.h"

void UpdatePlayer(Player &player, int screenWidth, int screenHeight) {
    float delta = GetFrameTime();

    if ((IsKeyDown(KEY_W) && player.position.y > 0))
        player.position.y -= player.speed * delta;
    if ((IsKeyDown(KEY_S) && player.position.y < screenHeight - player.size))
        player.position.y += player.speed * delta;
    if ((IsKeyDown(KEY_A) && player.position.x > 0))
        player.position.x -= player.speed * delta;
    if ((IsKeyDown(KEY_D) && player.position.x < screenWidth - player.size))
        player.position.x += player.speed * delta;
}

void DrawPlayer(Player player) {
    DrawRectangle((int)player.position.x, (int)player.position.y, player.size,
                  player.size, LIGHTGRAY);
};
