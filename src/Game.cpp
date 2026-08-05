#include "Game.h"

#include "Bullet.h"
#include "Config.h"
#include "Enemy.h"
#include "Player.h"
#include "Sprite.h"
#include <memory>

Game::Game() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Space Shooter");
    SetTargetFPS(144);

    playerTexture = LoadTexture("assets/spaceships/spr_spaceship_01_animation.png");
    enemyTexture = LoadTexture("assets/enemy_spaceships/spr_enemy_spaceship_01_animation.png");
    bulletTexture = LoadTexture("assets/spaceships/bullets/spr_spaceship_bullet_02.png");
    bgTexture = LoadTexture("assets/backgrounds/spr_background_01.png");

    if (playerTexture.id == 0)
        TraceLog(LOG_WARNING, "Could not load player texture");
    auto p = std::make_unique<Player>(playerTexture, SCREEN_WIDTH, SCREEN_HEIGHT, 200.0f);

    player = p.get();
    things.push_back(std::move(p));
}

Game::~Game() {
    things.clear();
    UnloadTexture(playerTexture);
    UnloadTexture(enemyTexture);
    UnloadTexture(bulletTexture);
    UnloadTexture(bgTexture);
    CloseWindow();
}

void Game::Run() {
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        gameTime += deltaTime;

        SpawnEnemies(deltaTime);
        FireBullets(deltaTime);

        for (auto &thing : things) {
            if (thing->active)
                thing->Update(deltaTime);
        }

        ResolveCollisions();
        RemoveDead();

        BeginDrawing();
        ClearBackground(BLACK);

        DrawTexturePro(bgTexture, {0, 0, 256, 256}, {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, {0, 0}, 0.0f, WHITE);
        DrawFPS(750, 10);

        for (auto &thing : things) {
            if (thing->active)
                thing->Draw();
        }

        EndDrawing();
    }
}

void Game::SpawnEnemies(float deltaTime) {
    spawnTimer += deltaTime;
    if (spawnTimer < spawnInterval)
        return;
    spawnTimer = 0.0f;

    auto x = static_cast<float>(GetRandomValue(0, 760));
    auto speed = static_cast<float>(GetRandomValue(80, static_cast<int>(200 + gameTime * 5.0f)));
    int health = GetRandomValue(2, 5);

    things.push_back(std::make_unique<Enemy>(enemyTexture, Vector2{.x = x, .y = -30.0f}, health, speed));
}

void Game::FireBullets(float deltaTime) {
    fireTimer += deltaTime;
    if (!IsKeyDown(KEY_X) || fireTimer < 0.15f)
        return;
    fireTimer = 0.0f;

    float bulletX = player->position.x + player->size.x / 2.0f;
    Vector2 bulletSize = GetFrameSize(bulletTexture, 1.0f, SPRITE_SCALE);
    bulletX -= bulletSize.x / 2.0f;

    things.push_back(std::make_unique<Bullet>(bulletTexture, Vector2{.x = bulletX, .y = player->position.y}, 500.0f));
}

void Game::ResolveCollisions() const {
    for (auto &a : things) {
        if (!a->active)
            continue;
        for (auto &b : things) {
            if (!b->active)
                continue;
            if (a.get() == b.get())
                continue;

            if (!CheckCollisionRecs(a->bounds(), b->bounds()))
                continue;

            // Bullet hits Enemy
            if (a->type() == EntityType::BULLET && b->type() == EntityType::ENEMY) {
                const auto &bullet = dynamic_cast<Bullet &>(*a);
                auto &enemy = dynamic_cast<Enemy &>(*b);
                enemy.TakeDamage(bullet.damage);
                a->active = false; // bullet is used
            }

            // Enemy hits Player
            if (a->type() == EntityType::ENEMY && b->type() == EntityType::PLAYER) {
                auto &hitPlayer = dynamic_cast<Player &>(*b);
                hitPlayer.TakeDamage(1);
                a->active = false; // enemy died
            }
        }
    }
}

void Game::ResetGame() {
    things.clear();

    spawnTimer = 0.0f;
    fireTimer = 0.0f;
    gameTime = 0.0f;

    auto p = std::make_unique<Player>(playerTexture, SCREEN_WIDTH, SCREEN_WIDTH, 200.0f);
    player = p.get();
    things.push_back(std::move(p));
}

void Game::RemoveDead() {
    things.erase(
        std::remove_if(things.begin(), things.end(), [](const std::unique_ptr<Entity> &e) { return !e->active; }),
        things.end()
    );
}
