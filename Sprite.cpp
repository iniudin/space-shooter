#include "Sprite.h"
#include "raylib.h"

Vector2 GetFrameSize(
    const Texture2D &texture,
    const float frameCount,
    const float scale
) {
    return {
        .x = static_cast<float>(texture.width) / frameCount * scale,
        .y = static_cast<float>(texture.height) * scale,
    };
}
