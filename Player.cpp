#include "Player.h"
#include "raylib.h"

void UpdatePlayer(
    Player &player,
    const float screenWidth,
    const float screenHeight
) {
    const float delta = GetFrameTime();

    if ((IsKeyDown(KEY_UP) && player.position.y > 0))
        player.position.y -= player.speed * delta;
    if ((IsKeyDown(KEY_DOWN) &&
         player.position.y < screenHeight - player.size.y))
        player.position.y += player.speed * delta;
    if ((IsKeyDown(KEY_LEFT) && player.position.x > 0))
        player.position.x -= player.speed * delta;
    if ((IsKeyDown(KEY_RIGHT) &&
         player.position.x < screenWidth - player.size.x))
        player.position.x += player.speed * delta;
}

void DrawPlayer(
    const Player &player,
    const Texture2D &texture,
    const float frame
) {
    constexpr float frameCount = 4.0f;
    constexpr float frameMultiple = 2.0f;

    const float frameWidth = static_cast<float>(texture.width) / frameCount;
    const auto frameHeight = static_cast<float>(texture.height);

    Rectangle source = {
        .x = frame * frameWidth,
        .y = 0.0f,
        .width = frameWidth,
        .height = frameHeight,
    };

    Rectangle dest = {
        .x = player.position.x,
        .y = player.position.y,
        .width = frameWidth * frameMultiple,
        .height = frameHeight * frameMultiple,
    };

    constexpr Vector2 origin = {0.0f, 0.0f};

    DrawTexturePro(texture, source, dest, origin, 0.0f, WHITE);
};
