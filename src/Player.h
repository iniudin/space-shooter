#pragma once

#include "Entity.h"
#include "raylib.h"

class Player : public Entity {
  public:
    Player(const Texture2D &texture, float screenWidth, float screenHeight, float speed);
    void Update(float deltaTime) override;
    void Draw() const override;

    [[nodiscard]] EntityType type() const override { return EntityType::PLAYER; };

    void TakeDamage(int amount);
    [[nodiscard]] int Health() const { return health; };

  private:
    Texture2D texture;
    float screenWidth;
    float screenHeight;
    float speed;
    float animationTick = 0.0f;
    int health = 3;
};
