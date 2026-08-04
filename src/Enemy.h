#pragma once

#include "raylib.h"
#include "Entity.h"

class Enemy : public Entity
{
public:
    Enemy(const Texture2D& texture, Vector2 start, int health, float speed);

    void Update(float deltaTime) override;
    void Draw() const override;
    [[nodiscard]] EntityType type() const override { return EntityType::ENEMY; };

    [[nodiscard]] int Health() const { return health; };
    void TakeDamage(int amount);

private:
    Texture2D texture;
    int health;
    float speed;
    float animationTick = 0.0f;
};
