#include "Bullet.h"

#include "Config.h"
#include "Sprite.h"

constexpr float BULLET_FRAME_COUNT = 1.0f;

Bullet::Bullet(const Texture2D &texture, const Vector2 start, const float speed) : texture(texture), speed(speed) {
    position = start;
    size = GetFrameSize(texture, BULLET_FRAME_COUNT, SPRITE_SCALE);
}

void Bullet::Update(float deltaTime) {
    position.y -= speed * deltaTime;
    if (position.y + size.y < 0.0f)
        active = false;
}

void Bullet::Draw() const {
    Rectangle source = {
        .x = 0.0f,
        .y = 0.0f,
        .width = size.x,
        .height = size.y,
    };
    Rectangle dest = bounds();
    DrawTexturePro(texture, source, dest, {.x = 0, .y = 0}, 0.0f, WHITE);
}
