#pragma once
#include "raylib.h"

enum EntityType {
    PLAYER,
    ENEMY,
    BULLET,
};

class Entity {
  public:
    Vector2 position{};
    Vector2 size{};
    bool active = true;

    Entity() = default;
    virtual ~Entity() = default;

    [[nodiscard]] virtual EntityType type() const = 0;
    [[nodiscard]] virtual Rectangle bounds() const;

    virtual void Update(float deltaTime) = 0;
    virtual void Draw() const = 0;
};

inline Rectangle Entity::bounds() const {
    return {.x = position.x, .y = position.y, .width = size.x, .height = size.y};
}
