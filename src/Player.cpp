#include "Player.h"
#include "Config.h"
#include "raylib.h"
#include "Sprite.h"

constexpr Vector2 PLAYER_POSITION = {
    .x = 400.0f,
    .y = 400.0f,
};

Player::Player(
    const Texture2D &texture,
    const float screenWidth,
    const float screenHeight,
    const float speed
) :
    texture(texture),
    screenWidth(screenWidth),
    screenHeight(screenHeight),
    speed(speed) {
    position = PLAYER_POSITION;
    size = GetFrameSize(texture, FRAME_COUNT, SPRITE_SCALE);
};

void Player::Update(float deltaTime) {
    if ((IsKeyDown(KEY_UP) && position.y > 0))
        position.y -= speed * deltaTime;
    if ((IsKeyDown(KEY_DOWN) &&
         position.y < screenHeight - size.y))
        position.y += speed * deltaTime;
    if ((IsKeyDown(KEY_LEFT) && position.x > 0))
        position.x -= speed * deltaTime;
    if ((IsKeyDown(KEY_RIGHT) &&
         position.x < screenWidth - size.x))
        position.x += speed * deltaTime;

    animationTick += deltaTime;
}

void Player::Draw() const {
    const float frameWidth = static_cast<float>(texture.width) / FRAME_COUNT;
    const auto frameHeight = static_cast<float>(texture.height);
    const int frame = static_cast<int>(animationTick / 0.1f) % static_cast<int>(FRAME_COUNT);

    const Rectangle source = {
        .x = static_cast<float>(frame) * frameWidth,
        .y = 0.0f,
        .width = frameWidth,
        .height = frameHeight,
    };

    const Rectangle dest = bounds();
    DrawTexturePro(texture, source, dest, {.x = 0, .y = 0}, 0.0f, WHITE);
}

void Player::TakeDamage(int amount) {
    health -= amount;
    if (health <= 0)
        active = false;
}
