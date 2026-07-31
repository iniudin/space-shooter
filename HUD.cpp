#include "HUD.h"
#include "Player.h"
#include "raylib.h"

void DrawSpeed(Player player) {
    DrawText(TextFormat("Speed %.0f", player.speed), 10, 10, 30, LIGHTGRAY);
};