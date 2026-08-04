#pragma once


#include "Entity.h"

class Bullet : public Entity
{
public:
    int damage = 1;
    Bullet(const Texture2D& texture, Vector2 start, float speed);

    void Update(float deltaTime) override;
    void Draw() const override;
    [[nodiscard]] EntityType type() const override { return EntityType::BULLET; };

private:
    Texture2D texture;
    float speed;
};
