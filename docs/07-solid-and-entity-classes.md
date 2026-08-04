# Chapter 07 — SOLID & Abstract Entity Classes

## What You Will Learn

- What a "God Function" is and why your `main.cpp` has become one
- What the five SOLID principles mean — each one tied to a real change in this
  game
- What an abstract class is and how to write one in C++ (`virtual`, `= 0`,
  `override`)
- How to create an `Entity` base class that Player, Enemy, and Bullet inherit
  from
- What a virtual destructor is and why you must have one
- How to store different entity types in one container with
  `std::vector<std::unique_ptr<Entity>>`
- How to write a `Game` class that replaces the god function
- How collision works when everything is behind an abstract base
- Common traps: fat base classes, `dynamic_cast`, abstraction for no reason

---

## The Plan

By the end of this chapter your game will play **exactly the same** — same
enemies, same bullets, same sprites. But the code will be organized completely
differently:

1. An abstract `Entity` base class with `Update()` and `Draw()`
2. `Player`, `Enemy`, and `Bullet` as concrete classes that inherit from
   `Entity`
3. A `Game` class that owns the window, textures, entities, and the game loop
4. A `main.cpp` that is only 3 lines long

You will build it in this order:

1. Look at the current code and understand what is wrong with it
2. Learn the SOLID principles (the "why")
3. Create the `Entity` base class
4. Convert `Player` to a class that inherits from `Entity`
5. Convert `Enemy` the same way
6. Convert `Bullet` the same way
7. Create the `Game` class that ties everything together
8. Handle collision between different entity types
9. Shrink `main.cpp` to 3 lines

---

## Part 1 — What is wrong with `main.cpp` right now?

Open your `main.cpp`. Count the jobs that one function does:

1. Creates the window
2. Loads all four textures
3. Creates the player and sets its fields
4. Runs the enemy spawner on a timer
5. Fires bullets when you press X
6. Calls `UpdatePlayer`, `UpdateEnemies`, `UpdateBullets` — each by name
7. Calls `DrawPlayer`, `DrawEnemies`, `DrawBullets` — each by name
8. Draws the background and HUD
9. Unloads textures

That is a **God Function** — one function doing too many unrelated jobs. The
problem is not that it "looks bad". The problem is practical:

- **Adding a new thing is hard.** Want to add a PowerUp? You need to add a
  struct, a vector, a spawn timer, an Update call, a Draw call, collision
  checks — all inside `main.cpp`. You will forget one of those steps.
- **Every type is handled by name.** The code says `UpdateEnemies(enemies, …)`,
  `UpdateBullets(bullets)`, `DrawPlayer(player, …)`. There is no way to say
  "update everything" in one line — you must list each type by hand.

The fix: give each object its own `Update()` and `Draw()`, then store them all
in one list and loop over it. That is what an abstract class lets you do.

---

## Part 2 — The five SOLID principles

SOLID is five rules for organizing classes. You do not need to memorize them —
just know what each one feels like in your game:

| Letter | Name | What it means in this game |
|---|---|---|
| **S** | Single Responsibility | Each class does one job. `Game` runs the loop. `Enemy` knows how to move and draw itself. `main.cpp` does not do everything. |
| **O** | Open/Closed | To add a new enemy type, you write a new class. You do **not** edit the game loop. |
| **L** | Liskov Substitution | A `ZigzagEnemy` can go anywhere an `Entity` can. The loop does not care which subclass it is. |
| **I** | Interface Segregation | The base class stays thin. Do not put `health` in the base — Bullet does not have health. |
| **D** | Dependency Inversion | The game loop depends on `Entity` (the abstraction), not on `Player`/`Enemy`/`Bullet` by name. |

You will feel each of these as you build the code below.

---

## Part 3 — The abstract base class: `Entity`

### What your structs look like now

Here is what you have today. Look at the pattern:

```cpp
// Player.h                       // Enemy.h                        // Bullet.h
struct Player {                   struct Enemy {                    struct Bullet {
    Vector2 position;                 Vector2 position;                 Vector2 position;
    Vector2 size;                     Vector2 size;                     Vector2 size;
    float speed;                      float speed;                      float speed;
};                                    int health;                       int damage;
                                  };                                };
```

All three have `position`, `size`, and `speed`. All three get updated every
frame. All three get drawn every frame. The difference is *how* — the player
moves with arrow keys, the enemy falls down, the bullet flies up. But the
shape is the same: **something on the field that updates and draws.**

That shared shape is your abstract class.

### What `Entity` will hold

Only the fields that **every** entity needs:

```
position  — where it is (Vector2)
size      — how big it is (Vector2)
active    — is it alive? (bool)
```

And three methods every entity must provide:

```
Update(dt)  — move yourself for this frame
Draw()      — draw yourself
kind()      — "I am a Player / Enemy / Bullet" (for collision)
```

**Why not `speed`, `health`, or `texture`?**

- A PowerUp might not move → no `speed`
- A Bullet has no health → no `health`
- Each child has its own texture → no shared `texture`

Putting those in the base would force every child to carry fields it does not
use. That violates **I** (Interface Segregation): keep the base thin.

### Create `Entity.h`

Create a new file `Entity.h`:

```cpp
#pragma once
#include "raylib.h"

// A tag that identifies what kind of entity this is.
// Used for collision — explained in Part 8.
enum class EntityKind {
    Player,
    Enemy,
    Bullet,
};

// Abstract base class: "an object on the field that updates and draws."
// You CANNOT create an Entity directly — only subclasses.
class Entity {
public:
    Vector2 position{};
    Vector2 size{};
    bool active = true;

    Entity() = default;
    virtual ~Entity() = default;          // REQUIRED — explained below

    virtual void Update(float dt) = 0;    // pure virtual — explained below
    virtual void Draw() const = 0;        // pure virtual
    virtual EntityKind kind() const = 0;  // pure virtual

    Rectangle bounds() const;             // shared: the collision rectangle
};

// Inline because it is so small — no .cpp file needed.
inline Rectangle Entity::bounds() const {
    return { position.x, position.y, size.x, size.y };
}
```

### Three new C++ concepts — explained

**1. `virtual void Update(float dt) = 0;` — pure virtual function**

The `= 0` means "this class does NOT provide a body for this function. Every
subclass MUST write its own." A class with even one `= 0` function is called
**abstract** — you cannot create it directly:

```cpp
Entity e;  // ❌ compile error — Entity is abstract
```

This is exactly what we want. `Entity` is a *concept* (something that updates
and draws), not a real object. Only `Player`, `Enemy`, `Bullet` are real.

**2. `virtual` — lets subclasses override**

When you write `virtual void Draw() const = 0;` in the base, and then a
subclass writes `void Draw() const override { ... }`, the correct version is
called even through a base pointer:

```cpp
Entity* e = new Enemy(…);
e->Draw();  // calls Enemy::Draw, not Entity::Draw
```

Without `virtual`, the compiler would call `Entity::Draw` (which does not exist)
and your program would be wrong.

**3. `virtual ~Entity() = default;` — virtual destructor**

If you hold an `Entity*` that actually points to an `Enemy`, and you delete it:

```cpp
Entity* e = new Enemy(…);
delete e;  // which destructor runs?
```

Without `virtual` on the destructor, only `Entity`'s destructor runs — the
`Enemy` part is never cleaned up. That is **undefined behavior**.

With `virtual ~Entity()`, the correct destructor (Enemy's, then Entity's) is
called. Rule: **any class used as a polymorphic base gets a virtual
destructor.**

---

## Part 4 — Convert `Player` to inherit from `Entity`

This is the biggest child because the Player has keyboard input, screen
clamping, and animation. Take it step by step.

### `Player.h` — the new version

Replace the entire file:

```cpp
#pragma once
#include "Entity.h"

class Player : public Entity {
public:
    // Constructor: needs the texture (for size), screen bounds, and speed.
    Player(Texture2D texture, float screenW, float screenH, float speed);

    void Update(float dt) override;
    void Draw() const override;
    EntityKind kind() const override { return EntityKind::Player; }

    void TakeDamage(int amount);
    int Health() const { return health; }

private:
    Texture2D texture;
    float screenW;
    float screenH;
    float speed;
    float animTimer = 0.0f;
    int health = 3;
};
```

**What changed from the old `struct Player`?**

| Before (struct) | After (class) |
|---|---|
| `position`, `size`, `speed` were public fields you set from `main.cpp` | `position` and `size` come from `Entity`. `speed` is private — the Player manages itself. |
| `UpdatePlayer(player, screenW, screenH)` was a free function | `player.Update(dt)` — the Player updates itself |
| `DrawPlayer(player, texture, frame)` was a free function | `player.Draw()` — the Player draws itself, it owns its texture |
| No health | `health` added (we will use it in collision) |

### `Player.cpp` — the new version

Replace the entire file:

```cpp
#include "Player.h"
#include "Sprite.h"

Player::Player(Texture2D texture, float screenW, float screenH, float speed)
    : texture(texture), screenW(screenW), screenH(screenH), speed(speed)
{
    position = { 400.0f, 400.0f };
    size = GetFrameSize(texture, 4.0f, 2.0f);  // 4 frames, 2x scale
}

void Player::Update(float dt) {
    if (IsKeyDown(KEY_UP)    && position.y > 0.0f)
        position.y -= speed * dt;
    if (IsKeyDown(KEY_DOWN)  && position.y < screenH - size.y)
        position.y += speed * dt;
    if (IsKeyDown(KEY_LEFT)  && position.x > 0.0f)
        position.x -= speed * dt;
    if (IsKeyDown(KEY_RIGHT) && position.x < screenW - size.x)
        position.x += speed * dt;

    animTimer += dt;
}

void Player::Draw() const {
    float frameWidth = (float)texture.width / 4.0f;
    float frameHeight = (float)texture.height;
    int frame = (int)(animTimer / 0.1f) % 4;

    Rectangle source = {
        frame * frameWidth, 0.0f,
        frameWidth, frameHeight
    };
    Rectangle dest = bounds();  // same rectangle as collision!
    DrawTexturePro(texture, source, dest, {0, 0}, 0.0f, WHITE);
}

void Player::TakeDamage(int amount) {
    health -= amount;
    if (health <= 0) active = false;
}
```

Notice:

- **`Update` and `Draw` own all their logic.** No free function, no passing the
  texture around from main.
- **`Draw()` uses `bounds()`** for the destination rectangle — the same
  rectangle used for collision. Drawing and collision can never disagree (the
  lesson from Chapter 06).
- **The constructor sets `position` and `size`** — `main.cpp` will never touch
  those fields directly.

---

## Part 5 — Convert `Enemy` to inherit from `Entity`

### `Enemy.h` — the new version

Replace the entire file:

```cpp
#pragma once
#include "Entity.h"

class Enemy : public Entity {
public:
    Enemy(Texture2D texture, Vector2 start, int health, float speed);

    void Update(float dt) override;
    void Draw() const override;
    EntityKind kind() const override { return EntityKind::Enemy; }

    int Health() const { return health; }
    void TakeDamage(int amount);

private:
    Texture2D texture;
    int health;
    float speed;
    float animTimer = 0.0f;
};
```

### `Enemy.cpp` — the new version

Replace the entire file:

```cpp
#include "Enemy.h"
#include "Sprite.h"

Enemy::Enemy(Texture2D texture, Vector2 start, int health, float speed)
    : texture(texture), health(health), speed(speed)
{
    position = start;
    size = GetFrameSize(texture, 4.0f, 2.0f);  // 4 frames, 2x scale
}

void Enemy::Update(float dt) {
    position.y += speed * dt;      // fall down
    animTimer += dt;
    if (position.y > 480.0f) active = false;  // fell off the bottom
}

void Enemy::Draw() const {
    float frameWidth = (float)texture.width / 4.0f;
    float frameHeight = (float)texture.height;
    int frame = (int)(animTimer / 0.1f) % 4;

    Rectangle source = {
        frame * frameWidth, frameHeight,
        frameWidth, -frameHeight       // negative height = flip vertically
    };
    Rectangle dest = bounds();
    DrawTexturePro(texture, source, dest, {0, 0}, 0.0f, WHITE);
}

void Enemy::TakeDamage(int amount) {
    health -= amount;
    if (health <= 0) active = false;
}
```

This is the same draw logic you had in `DrawEnemies`, but now each Enemy owns
its own `animTimer` — so enemies no longer all flicker in sync. (Remember
Homework 4 from Chapter 06? This solves it by design.)

---

## Part 6 — Convert `Bullet` to inherit from `Entity`

### `Bullet.h` — the new version

Replace the entire file:

```cpp
#pragma once
#include "Entity.h"

class Bullet : public Entity {
public:
    int damage = 1;

    Bullet(Texture2D texture, Vector2 start, float speed);

    void Update(float dt) override;
    void Draw() const override;
    EntityKind kind() const override { return EntityKind::Bullet; }

private:
    Texture2D texture;
    float speed;
};
```

### `Bullet.cpp` — the new version

Replace the entire file:

```cpp
#include "Bullet.h"
#include "Sprite.h"

Bullet::Bullet(Texture2D texture, Vector2 start, float speed)
    : texture(texture), speed(speed)
{
    position = start;
    size = GetFrameSize(texture, 1.0f, 2.0f);  // 1 frame, 2x scale
}

void Bullet::Update(float dt) {
    position.y -= speed * dt;            // fly up
    if (position.y + size.y < 0.0f) active = false;  // left the top
}

void Bullet::Draw() const {
    Rectangle source = {
        0.0f, 0.0f,
        (float)texture.width, (float)texture.height
    };
    Rectangle dest = bounds();
    DrawTexturePro(texture, source, dest, {0, 0}, 0.0f, WHITE);
}
```

Bullet is the simplest child — no animation, no health. It moves up, draws
itself, and dies when it leaves the screen.

---

## Part 7 — The `Game` class

Now you need something to own the window, the textures, the entity list, and
the game loop. That is the `Game` class. It replaces the god function in
`main.cpp`.

### What is inside `Game`?

Before you write code, here is everything the `Game` class will hold:

```
Fields:
  screenW, screenH        — window size (float)
  playerTexture           — Texture2D
  enemyTexture            — Texture2D
  bulletTexture           — Texture2D
  bgTexture               — Texture2D
  things                  — std::vector<std::unique_ptr<Entity>>  (ALL entities)
  player                  — Player*  (raw pointer into `things`, for input/shooting)
  spawnTimer              — float
  spawnInterval           — float
  fireTimer               — float
  gameTime                — float

Methods:
  Game()                  — create window, load textures, create the player
  ~Game()                 — unload textures, close window
  Run()                   — the game loop: while (!WindowShouldClose())
  SpawnEnemies(float dt)  — the enemy spawner
  FireBullets(float dt)   — the bullet spawner
  ResolveCollisions()     — bullet vs enemy, enemy vs player
  RemoveDead()            — erase inactive entities
```

### `Game.h`

Create a new file `Game.h`:

```cpp
#pragma once
#include "Entity.h"
#include "Player.h"
#include <memory>
#include <vector>

class Game {
public:
    Game();
    ~Game();
    void Run();

private:
    void SpawnEnemies(float dt);
    void FireBullets(float dt);
    void ResolveCollisions();
    void RemoveDead();

    float screenW = 800.0f;
    float screenH = 450.0f;

    Texture2D playerTexture{};
    Texture2D enemyTexture{};
    Texture2D bulletTexture{};
    Texture2D bgTexture{};

    std::vector<std::unique_ptr<Entity>> things;
    Player* player = nullptr;   // points into `things` — do NOT delete

    float spawnTimer = 0.0f;
    float spawnInterval = 2.0f;
    float fireTimer = 0.0f;
    float gameTime = 0.0f;
};
```

**New concept: `std::unique_ptr<Entity>`**

In the old code you had three separate vectors:

```cpp
std::vector<Bullet> bullets;
std::vector<Enemy>  enemies;
Player player;
```

Now you want one vector that can hold Players, Enemies, and Bullets together.
But `std::vector<Entity>` will not work — `Entity` is abstract (you cannot
create one), and even if you could, storing a derived class by value would
**slice** it (lose the derived data).

The solution is to store pointers to the heap:

```cpp
std::vector<std::unique_ptr<Entity>> things;
```

- `std::unique_ptr<Entity>` is a smart pointer that **owns** one Entity on the
  heap. When the unique_ptr is destroyed, it automatically `delete`s the Entity.
  No manual `delete`, no leaks.
- Because it is a pointer to `Entity`, it can point to any subclass: `Player`,
  `Enemy`, or `Bullet`.
- You create them with `std::make_unique<Enemy>(…)` — explained below.

**Why `Player* player`?**

The player is special — you need to check keyboard input and fire bullets from
its position. So you keep a raw (non-owning) pointer to the Player inside
`things`. The `unique_ptr` in the vector owns it; this raw pointer just lets
you find it quickly.

### `Game.cpp`

Create a new file `Game.cpp`:

```cpp
#include "Game.h"
#include "Bullet.h"
#include "Enemy.h"
#include "HUD.h"
#include "Sprite.h"

// ─── Constructor: create window, load textures, make the player ───

Game::Game() {
    InitWindow(screenW, screenH, "Space Shooter");
    SetTargetFPS(144);

    playerTexture = LoadTexture(
        "assets/spaceships/spr_spaceship_01_animation.png");
    enemyTexture = LoadTexture(
        "assets/enemy_spaceships/spr_enemy_spaceship_01_animation.png");
    bulletTexture = LoadTexture(
        "assets/spaceships/bullets/spr_spaceship_bullet_02.png");
    bgTexture = LoadTexture(
        "assets/backgrounds/spr_background_01.png");

    if (playerTexture.id == 0)
        TraceLog(LOG_WARNING, "Could not load player texture!");

    // Create the player and add it to the entity list.
    auto p = std::make_unique<Player>(
        playerTexture, screenW, screenH, 200.0f);
    player = p.get();       // save a raw pointer before we move it
    things.push_back(std::move(p));
}

// ─── Destructor: unload textures, close window ───

Game::~Game() {
    things.clear();         // destroy all entities before unloading textures
    UnloadTexture(playerTexture);
    UnloadTexture(enemyTexture);
    UnloadTexture(bulletTexture);
    UnloadTexture(bgTexture);
    CloseWindow();
}

// ─── The game loop ───

void Game::Run() {
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        gameTime += dt;

        // --- UPDATE ---
        SpawnEnemies(dt);
        FireBullets(dt);

        for (auto& thing : things) {
            if (thing->active) thing->Update(dt);
        }

        ResolveCollisions();
        RemoveDead();

        // --- DRAW ---
        BeginDrawing();
        ClearBackground(BLACK);

        DrawTexturePro(
            bgTexture,
            {0, 0, 256, 256},
            {0, 0, screenW, screenH},
            {0, 0}, 0.0f, WHITE);

        for (auto& thing : things) {
            if (thing->active) thing->Draw();
        }

        int bulletCount = 0;
        for (auto& thing : things) {
            if (thing->kind() == EntityKind::Bullet && thing->active)
                bulletCount++;
        }
        DrawBulletCount(bulletCount);
        DrawSpeed((int)200);
        DrawFPS(750, 10);

        EndDrawing();
    }
}

// ─── Enemy spawner ───

void Game::SpawnEnemies(float dt) {
    spawnTimer += dt;
    if (spawnTimer < spawnInterval) return;
    spawnTimer = 0.0f;

    float x = (float)GetRandomValue(0, 760);
    float speed = (float)GetRandomValue(80, (int)(200 + gameTime * 5.0f));
    int health = GetRandomValue(2, 8);

    things.push_back(std::make_unique<Enemy>(
        enemyTexture, Vector2{x, -30.0f}, health, speed));
}

// ─── Bullet spawner ───

void Game::FireBullets(float dt) {
    fireTimer += dt;
    if (!IsKeyDown(KEY_X) || fireTimer < 0.15f) return;
    fireTimer = 0.0f;

    float bulletX = player->position.x + player->size.x / 2.0f;
    Vector2 bulletSize = GetFrameSize(bulletTexture, 1.0f, 2.0f);
    bulletX -= bulletSize.x / 2.0f;  // center on the player

    things.push_back(std::make_unique<Bullet>(
        bulletTexture, Vector2{bulletX, player->position.y}, 500.0f));
}

// ─── Collision ───

void Game::ResolveCollisions() {
    for (auto& a : things) {
        if (!a->active) continue;
        for (auto& b : things) {
            if (!b->active) continue;
            if (a.get() == b.get()) continue;  // same object

            if (!CheckCollisionRecs(a->bounds(), b->bounds())) continue;

            // Bullet hits Enemy
            if (a->kind() == EntityKind::Bullet &&
                b->kind() == EntityKind::Enemy)
            {
                Bullet& bullet = static_cast<Bullet&>(*a);
                Enemy&  enemy  = static_cast<Enemy&>(*b);
                enemy.TakeDamage(bullet.damage);
                a->active = false;  // bullet is consumed
            }

            // Enemy hits Player
            if (a->kind() == EntityKind::Enemy &&
                b->kind() == EntityKind::Player)
            {
                static_cast<Player&>(*b).TakeDamage(1);
                a->active = false;  // enemy is consumed
            }
        }
    }
}

// ─── Remove dead entities ───

void Game::RemoveDead() {
    things.erase(
        std::remove_if(things.begin(), things.end(),
            [](const std::unique_ptr<Entity>& e) { return !e->active; }),
        things.end());
}
```

This is the longest file in the project. But every method does **one job** — the
**S** in SOLID. Read each section's comment to see what it does.

### Three things to understand

**1. `std::make_unique<Enemy>(…)` — creating entities on the heap**

```cpp
things.push_back(std::make_unique<Enemy>(
    enemyTexture, Vector2{x, -30.0f}, health, speed));
```

`std::make_unique<Enemy>(…)` creates a `new Enemy(…)` on the heap and wraps it
in a `unique_ptr`. When this `unique_ptr` is removed from the vector (by
`RemoveDead` or when `Game` is destroyed), the Enemy is automatically deleted.

**2. `static_cast<Enemy&>(*b)` — safe because of the `kind()` check**

In `ResolveCollisions`, we already checked `b->kind() == EntityKind::Enemy`
before casting. We *know* it is an Enemy, so `static_cast` is correct and fast.

Do **not** use `dynamic_cast` here. It is slower and gives you nothing extra
when you already have the kind tag.

**3. The erase-remove idiom in `RemoveDead()`**

You cannot erase items from a vector while looping forward — it shifts
everything and you skip items (you learned this in Chapter 05). The
erase-remove idiom solves this:

```cpp
things.erase(
    std::remove_if(things.begin(), things.end(),
        [](const std::unique_ptr<Entity>& e) { return !e->active; }),
    things.end());
```

- `std::remove_if` moves all the "dead" items to the end of the vector and
  returns an iterator to where they start.
- `.erase(…)` chops off that tail.
- The `[](…){ return !e->active; }` part is a **lambda** — a small inline
  function. It says "remove the ones that are not active."

---

## Part 8 — Collision and the `kind()` tag

Collision is the one place where "just use the base class" does **not** cleanly
work. Why?

A collision is an interaction between **two specific kinds** — bullet vs enemy
is different from enemy vs player. The *effect* depends on which pair collided.
That is not something `Entity::Update()` can handle alone.

The solution is the `kind()` tag:

```cpp
enum class EntityKind { Player, Enemy, Bullet };
```

Each subclass returns its own kind:

```cpp
// In Enemy:
EntityKind kind() const override { return EntityKind::Enemy; }
```

Then `ResolveCollisions` checks the pair:

```cpp
if (a->kind() == EntityKind::Bullet && b->kind() == EntityKind::Enemy) {
    // bullet hit enemy — deal damage, consume bullet
}
```

**Three rules for collision:**

1. **Use `kind()` + `static_cast`, not `dynamic_cast`.** The tag already tells
   you the type. `static_cast` is fast and obvious.
2. **Handle only the pairs that matter.** Bullet-vs-Enemy, Enemy-vs-Player.
   Everything else (Bullet-vs-Bullet, Player-vs-Player) is ignored.
3. **Only mark things as dead during collision** (`active = false`). Do not
   erase from the vector while looping. `RemoveDead()` handles that afterwards.

This is honestly where abstraction ends. Collision is a **relationship** between
two concrete types — it cannot live inside one base class. Real game engines
handle it the same way: a tag or layer system separate from the class hierarchy.

---

## Part 9 — Shrink `main.cpp` to 3 lines

Replace the entire `main.cpp`:

```cpp
#include "Game.h"

int main() {
    Game game;
    game.Run();
    return 0;
}
```

That is it. The `Game` constructor creates the window and loads textures.
`Run()` runs the game loop. The `Game` destructor unloads everything.

This is **S** (Single Responsibility) at the highest level — `main()` does one
thing: start the game.

---

## Part 10 — Build and run

Your file structure should now look like this:

```
space-shooter/
├── main.cpp            ← 3 lines
├── Game.h              ← NEW — game loop, spawners, collision
├── Game.cpp            ← NEW
├── Entity.h            ← NEW — abstract base class
├── Player.h            ← CHANGED — now inherits from Entity
├── Player.cpp          ← CHANGED
├── Enemy.h             ← CHANGED — now inherits from Entity
├── Enemy.cpp           ← CHANGED
├── Bullet.h            ← CHANGED — now inherits from Entity
├── Bullet.cpp          ← CHANGED
├── Sprite.h/.cpp       ← unchanged
├── HUD.h/.cpp          ← unchanged
├── Makefile            ← unchanged (wildcard *.cpp picks up new files)
└── assets/
```

Build and run. The game should play exactly the same — same enemies falling,
same bullets shooting, same sprites. The difference is entirely in how the code
is organized.

If you get a linker error about undefined `Game::…`, make sure `Game.cpp` is
being compiled. If your Makefile uses `wildcard *.cpp`, it should be automatic.

---

## Part 11 — Feel Open/Closed: add `ZigzagEnemy`

Now feel the payoff. Add a new enemy type that moves side to side as it falls.
You will write **one new file** and change **zero lines** in the game loop.

### `ZigzagEnemy.h`

Create a new file:

```cpp
#pragma once
#include "Enemy.h"

class ZigzagEnemy : public Enemy {
public:
    ZigzagEnemy(Texture2D texture, Vector2 start, int health, float speed)
        : Enemy(texture, start, health, speed),
          moveRight(GetRandomValue(0, 1) == 1) {}

    void Update(float dt) override {
        Enemy::Update(dt);  // still fall down + animate
        position.x += 60.0f * dt * (moveRight ? 1.0f : -1.0f);
        if (position.x < 0.0f) moveRight = true;
        if (position.x > 760.0f) moveRight = false;
    }

private:
    bool moveRight;
};
```

Now spawn a mix in `Game::SpawnEnemies`:

```cpp
void Game::SpawnEnemies(float dt) {
    spawnTimer += dt;
    if (spawnTimer < spawnInterval) return;
    spawnTimer = 0.0f;

    float x = (float)GetRandomValue(0, 760);
    float speed = (float)GetRandomValue(80, (int)(200 + gameTime * 5.0f));
    int health = GetRandomValue(2, 8);

    // 50% chance of zigzag
    if (GetRandomValue(0, 1) == 0) {
        things.push_back(std::make_unique<Enemy>(
            enemyTexture, Vector2{x, -30.0f}, health, speed));
    } else {
        things.push_back(std::make_unique<ZigzagEnemy>(
            enemyTexture, Vector2{x, -30.0f}, health, speed));
    }
}
```

The `Run()` loop — the Update/Draw loops — **did not change at all.** The new
enemy type just works because it is an `Entity`. That is **O** (Open/Closed):
open for extension (new class), closed for modification (no loop edits).

And it works because of **L** (Liskov): a `ZigzagEnemy` behaves like an
`Entity`, so the loop handles it correctly.

---

## Common Mistakes

### Mistake 1 — Abstraction theatre

You make `Entity` but then write `if (thing->kind() == EntityKind::Enemy)` in
the update loop. If you are checking types everywhere, the abstraction gave you
nothing. The only place you should check `kind()` is collision — everywhere else,
use the interface (`Update`, `Draw`).

### Mistake 2 — Missing the virtual destructor

`std::unique_ptr<Entity>` calls `delete` through an `Entity*`. Without
`virtual ~Entity()`, the subclass destructor never runs — that is undefined
behavior. **Any polymorphic base class needs a virtual destructor.**

### Mistake 3 — Fat base class

Pulling `health`, `texture`, `speed` into `Entity` because "it's easier" forces
every child to carry fields it does not use. A Bullet has no health. A static
PowerUp has no speed. Keep the base thin — only what truly every entity shares.

### Mistake 4 — `dynamic_cast` habit

`dynamic_cast` is slow and tempting. Use the `kind()` tag + `static_cast` for
collision. It is faster, and the tag already tells you the type.

### Mistake 5 — Erasing while iterating

Never erase from a vector while looping forward through it. Mark dead entities
(`active = false`) during the loop, then erase them all at once afterwards with
the erase-remove idiom. You learned this in Chapter 05 and it still applies.

---

## Homework

Build and run after each one.

**Homework 1 — The full migration.**
Implement everything in this chapter. Replace all the old struct + free function
code with the new Entity/Player/Enemy/Bullet/Game classes. The game must play
identically. Commit when it compiles.

**Homework 2 — Add `ZigzagEnemy`.**
Follow Part 11. Spawn a mix of plain and zigzag enemies. Confirm the game loop
did not change. That is Open/Closed.

**Homework 3 — Add `ArmoredEnemy`.**
Create `class ArmoredEnemy : public Enemy`. It ignores the first hit for 2
seconds after spawning (an `armorTimer` checked against the elapsed time).
`ResolveCollisions` still uses only `Enemy&` — the `TakeDamage` override in
`ArmoredEnemy` handles the armor logic. That is **L** (Liskov): swap in a new
subclass and the collision code stays correct.

**Homework 4 — Add `PowerUp`.**
Create `class PowerUp : public Entity`. It falls slowly and gives the player a
speed boost on contact. It has no `health`. If you catch yourself adding
`health` to `Entity` "just for the PowerUp", stop — you are breaking **I**
(Interface Segregation). Keep the base thin.

**Homework 5 — Feel the old way.**
Before deleting your git history, look at the old `main.cpp` (use `git diff`
or `git stash`). Count the lines. Count the lines in the new `main.cpp`. Count
how many files you would need to edit to add a new enemy type in the old code
vs the new code. That difference is what SOLID gives you.

---

## Next

**Chapter 08 — Sound & Music.** Now that entities sit behind a single
interface, adding things that move and draw is cheap. Next: things that make
sound — `InitAudioDevice`, `LoadSound`, `PlaySound`, `LoadMusicStream`, and an
explosion SFX triggered from the collision pass when an entity dies.
