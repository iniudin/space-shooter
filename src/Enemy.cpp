#include "Enemy.h"
#include "Config.h"
#include "raylib.h"
#include "Sprite.h"


Enemy::Enemy(const Texture2D& texture, Vector2 start, int health, float speed) : texture(texture),
    health(health),
    speed(speed)
{
    position = start;
    size = GetFrameSize(texture, FRAME_COUNT, SPRITE_SCALE);
}

void Enemy::Update(const float deltaTime)
{
    position.y += speed * deltaTime;
    animationTick += deltaTime;
    if (position.y > SCREEN_HEIGHT - size.y) active = false;
}

void Enemy::Draw() const
{
    const float frameWidth = static_cast<float>(texture.width) / FRAME_COUNT;
    const auto frameHeight = static_cast<float>(texture.height);
    const int frame = static_cast<int>(animationTick / 0.1f) % static_cast<int>(FRAME_COUNT);

    const Rectangle source = {
        .x = static_cast<float>(frame) * frameWidth,
        .y = 0.0f,
        .width = frameWidth,
        .height = -frameHeight,
    };

    const Rectangle dest = bounds();
    DrawTexturePro(texture, source, dest, {.x = 0, .y = 0}, 0.0f, WHITE);
}

void Enemy::TakeDamage(int amount)
{
    health -= amount;
    if (health <= 0) active = false;
}
