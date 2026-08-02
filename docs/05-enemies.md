# Chapter 05 — Enemies

## What You Will Learn

- How to define an `Enemy` struct with position, speed, and health
- How to spawn enemies from outside the top of the screen
- How to move enemies downward every frame
- How to detect collision between a bullet and an enemy
- How to remove enemies that are hit or that leave the screen
- What a rectangle collision check is and the math behind it
- How to use a spawn timer to control the rate of enemy appearance

---

## The Plan

By the end of this chapter your game will:

1. Spawn enemies that fall from the top of the screen
2. Let your bullets destroy them on contact
3. Remove enemies that reach the bottom without being hit

You will build it in this order:

1. Create an `Enemy` struct and its files
2. Spawn enemies on a timer in `main()`
3. Move enemies downward in `UpdateEnemies()`
4. Remove enemies that go off screen
5. Check collision between bullets and enemies
6. Remove enemies and bullets that collide

---

## What an Enemy Needs

Before writing code, think about what data an enemy must carry:

- **Where is it?** → `position` (a `Vector2`)
- **How fast does it move?** → `speed` (a `float`)
- **How much health does it have?** → `health` (an `int`)

That is all. Start small. You can add more fields later.

```cpp
struct Enemy {
    Vector2 position;
    float speed;
    int health;
};
```

Size and color are not fields here because all enemies will be
the same size and color for now. Only store data that varies.

---

## Step by Step — Do This Now

### Step 1 — Create `Enemy.h`

Create a new file `Enemy.h`:

```cpp
#pragma once

#include "raylib.h"
#include <vector>

#include "Bullet.h"

struct Enemy {
    Vector2 position;
    float speed;
    int health;
};

void UpdateEnemies(std::vector<Enemy>& enemies, std::vector<Bullet>& bullets);
void DrawEnemies(const std::vector<Enemy>& enemies);
```

Notice that `Enemy.h` includes `Bullet.h`.
That is because `UpdateEnemies` needs to know what a `Bullet` is —
it will check if any bullet has hit any enemy.

The function signatures follow the same pattern as `Bullet.h`:

- `UpdateEnemies` takes both lists **by reference** (`&`) because it will modify both — removing hit enemies and hit bullets.
- `DrawEnemies` takes enemies as `const &` — it only reads, never writes.

---

### Step 2 — Create `Enemy.cpp`

Create a new file `Enemy.cpp`:

```cpp
#include "Enemy.h"
#include "raylib.h"

const int ENEMY_WIDTH  = 40;
const int ENEMY_HEIGHT = 30;

void UpdateEnemies(std::vector<Enemy>& enemies, std::vector<Bullet>& bullets) {
    float delta = GetFrameTime();

    // Move all enemies downward
    for (Enemy& e : enemies) {
        e.position.y += e.speed * delta;
    }

    // Check collision between each bullet and each enemy
    for (int i = (int)bullets.size() - 1; i >= 0; i--) {
        for (int j = (int)enemies.size() - 1; j >= 0; j--) {
            Rectangle bulletRect = {
                bullets[i].position.x,
                bullets[i].position.y,
                6,
                16
            };
            Rectangle enemyRect = {
                enemies[j].position.x,
                enemies[j].position.y,
                (float)ENEMY_WIDTH,
                (float)ENEMY_HEIGHT
            };

            if (CheckCollisionRecs(bulletRect, enemyRect)) {
                enemies[j].health -= 1;
                bullets.erase(bullets.begin() + i);

                if (enemies[j].health <= 0) {
                    enemies.erase(enemies.begin() + j);
                }

                break;
            }
        }
    }

    // Remove enemies that have gone off the bottom of the screen
    for (int i = (int)enemies.size() - 1; i >= 0; i--) {
        if (enemies[i].position.y > 450) {
            enemies.erase(enemies.begin() + i);
        }
    }
}

void DrawEnemies(const std::vector<Enemy>& enemies) {
    for (const Enemy& e : enemies) {
        DrawRectangle(
            (int)e.position.x,
            (int)e.position.y,
            ENEMY_WIDTH,
            ENEMY_HEIGHT,
            RED
        );
    }
}
```

There is a lot happening here. Each piece will be explained in detail below.

---

## Moving Enemies Downward

```cpp
for (Enemy& e : enemies) {
    e.position.y += e.speed * delta;
}
```

This is the same pattern as bullets, but in the opposite direction.

- Bullets use `y -= speed * delta` — they move **up** (y decreases).
- Enemies use `y += speed * delta` — they move **down** (y increases).

Remember the coordinate system from Chapter 02:

```
(0,0) ──────────── x →
  │
  │
  y    y = 0 is the top of the screen
  ↓    y = 450 is the bottom
```

Adding to `y` pushes things toward the bottom. Subtracting pulls them toward the top.

---

## What is a Rectangle Collision Check?

Before you can understand `CheckCollisionRecs`, you need to understand
what a collision between two rectangles means geometrically.

Two rectangles are **overlapping** when they are NOT separated.

They are separated when at least one of these is true:

```
A is entirely to the RIGHT of B  →  A.x > B.x + B.width
A is entirely to the LEFT of B   →  A.x + A.width < B.x
A is entirely BELOW B             →  A.y > B.y + B.height
A is entirely ABOVE B             →  A.y + A.height < B.y
```

If **none** of those are true, the rectangles overlap.

Raylib's `CheckCollisionRecs` does exactly this check for you.
You give it two `Rectangle` values and it returns `true` if they overlap.

### What is a `Rectangle` in raylib?

`Rectangle` is a raylib type that holds four values:

```cpp
Rectangle r;
r.x      = 100;   // left edge in pixels
r.y      = 200;   // top edge in pixels
r.width  = 40;    // how wide
r.height = 30;    // how tall
```

Or in the shorter initialization form:

```cpp
Rectangle r = { 100, 200, 40, 30 };
//              x    y    w   h
```

The order is always: **x, y, width, height**.

### Building the rectangles for collision

A bullet is 6 pixels wide and 16 pixels tall.
An enemy is `ENEMY_WIDTH` wide and `ENEMY_HEIGHT` tall.

```cpp
Rectangle bulletRect = {
    bullets[i].position.x,   // left edge of the bullet
    bullets[i].position.y,   // top edge of the bullet
    6,                        // bullet width
    16                        // bullet height
};

Rectangle enemyRect = {
    enemies[j].position.x,   // left edge of the enemy
    enemies[j].position.y,   // top edge of the enemy
    (float)ENEMY_WIDTH,       // enemy width
    (float)ENEMY_HEIGHT       // enemy height
};
```

`Rectangle` fields are `float`, so the integer constants need `(float)` casts.

---

## The Nested Collision Loop

```cpp
for (int i = (int)bullets.size() - 1; i >= 0; i--) {
    for (int j = (int)enemies.size() - 1; j >= 0; j--) {
        // check bullet[i] against enemy[j]
    }
}
```

This is a **nested loop** — a loop inside a loop.

The outer loop goes through every bullet.
For each bullet, the inner loop goes through every enemy.
Together, every bullet is checked against every enemy.

If you have 3 bullets and 4 enemies, that is 3 × 4 = 12 checks total.
In a real game with hundreds of objects you would use smarter methods,
but for now this is perfectly fine.

**Both loops go backward** for the same reason as the bullet cleanup loop
in Chapter 04: if you erase an item while looping forward, you skip the
item after it. Going backward avoids that problem entirely.

### What happens when a hit is detected

```cpp
if (CheckCollisionRecs(bulletRect, enemyRect)) {
    enemies[j].health -= 1;           // deal 1 damage to the enemy
    bullets.erase(bullets.begin() + i); // destroy the bullet

    if (enemies[j].health <= 0) {
        enemies.erase(enemies.begin() + j); // destroy the enemy if dead
    }

    break;  // ← this bullet is gone, stop checking other enemies
}
```

**Why `break`?**

After erasing `bullets[i]`, that bullet no longer exists.
If you continued the inner loop, the next iteration would try to
use `bullets[i]` again — but it has been deleted. That is undefined
behavior and can cause a crash.

`break` exits the inner loop immediately.
The outer loop then decrements `i` and moves to the previous bullet.

**Why `health -= 1` before checking `<= 0`?**

This lets you easily create enemies that take more than one hit.
Right now enemies start with `health = 1`, so one hit kills them.
Later you could start them with `health = 3` and the same code
would require three bullets to destroy them.

---

## Removing Enemies That Leave the Screen

```cpp
for (int i = (int)enemies.size() - 1; i >= 0; i--) {
    if (enemies[i].position.y > 450) {
        enemies.erase(enemies.begin() + i);
    }
}
```

Same backward-loop pattern, same logic as bullet cleanup.
If an enemy's top edge passes y = 450 (the bottom of the screen),
it has left the screen and should be removed.

---

### Step 3 — Spawn Enemies in `main.cpp`

Spawning an enemy means creating a new `Enemy` and adding it to the list.
This happens on a timer — every N seconds, one new enemy appears.

The spawn position is off the top of the screen (y < 0) at a random x.
The enemy then moves downward and becomes visible as it enters the screen.

Add these to `main()`:

```cpp
float spawnTimer = 0.0f;
float spawnInterval = 1.5f;   // one enemy every 1.5 seconds
std::vector<Enemy> enemies;
```

Then inside the game loop, in the UPDATE section:

```cpp
spawnTimer += GetFrameTime();
if (spawnTimer >= spawnInterval) {
    Enemy e;
    e.position.x = (float)GetRandomValue(0, 760);  // random x on screen
    e.position.y = -30;                             // just above the top edge
    e.speed      = (float)GetRandomValue(80, 200);  // random speed
    e.health     = 1;
    enemies.push_back(e);
    spawnTimer = 0.0f;
}

UpdateEnemies(enemies, bullets);
```

### What is `GetRandomValue`?

```cpp
int GetRandomValue(int min, int max);
```

This is a raylib function that returns a random integer between `min` and `max`, inclusive.

```cpp
GetRandomValue(0, 760)   // returns a random number: 0, 1, 2, ... 759, or 760
GetRandomValue(80, 200)  // returns a random number between 80 and 200
```

**Why 760 for x?**
The screen is 800 pixels wide. The enemy is 40 pixels wide.
If x were 800, the enemy would start with its left edge at the right border,
which means it would be entirely off screen to the right.
Using `800 - ENEMY_WIDTH = 760` ensures the enemy always starts fully visible horizontally.

**Why -30 for y?**
The enemy is 30 pixels tall. Starting at y = -30 means its bottom edge is
exactly at y = 0 — the top of the screen. It enters smoothly from above,
rather than popping into existence mid-screen.

### Why `(float)GetRandomValue(...)`?

`GetRandomValue` returns an `int`. `e.position.x` and `e.speed` are `float`.
The cast converts the integer to a float so the types match.

---

### Step 4 — Update `main.cpp`

Your full `main.cpp` should now look like this:

```cpp
#include "raylib.h"
#include "Player.h"
#include "Bullet.h"
#include "Enemy.h"
#include "HUD.h"
#include <vector>

int main() {
    const int screenWidth  = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Pesawat Tempur");
    SetTargetFPS(90);

    Player player;
    player.position = { 400, 400 };
    player.size     = 40;
    player.speed    = 200.0f;

    std::vector<Bullet> bullets;
    std::vector<Enemy>  enemies;

    float fireTimer     = 0.0f;
    float spawnTimer    = 0.0f;
    float spawnInterval = 1.5f;

    while (!WindowShouldClose()) {
        // --- UPDATE ---
        UpdatePlayer(player, screenWidth, screenHeight);

        fireTimer += GetFrameTime();
        if (IsKeyDown(KEY_X) && fireTimer >= 0.15f) {
            Bullet b;
            b.position.x = player.position.x + player.size / 2.0f - 3;
            b.position.y = player.position.y;
            b.speed      = 500.0f;
            bullets.push_back(b);
            fireTimer = 0.0f;
        }

        spawnTimer += GetFrameTime();
        if (spawnTimer >= spawnInterval) {
            Enemy e;
            e.position.x = (float)GetRandomValue(0, 760);
            e.position.y = -30;
            e.speed      = (float)GetRandomValue(80, 200);
            e.health     = 1;
            enemies.push_back(e);
            spawnTimer = 0.0f;
        }

        UpdateEnemies(enemies, bullets);

        // --- DRAW ---
        BeginDrawing();
        ClearBackground(DARKGRAY);
        DrawPlayer(player);
        DrawBullets(bullets);
        DrawEnemies(enemies);
        DrawBulletCount((int)bullets.size());
        DrawSpeed((int)player.speed);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
```

---

### Step 5 — Update the Makefile

`Enemy.cpp` is a new source file. Add it to the build:

```makefile
SRCS = main.cpp Player.cpp HUD.cpp Bullet.cpp Enemy.cpp
```

If you forget this, the linker will complain:

```
Undefined symbols: "UpdateEnemies", "DrawEnemies"
```

That means: the compiler saw the declarations in `Enemy.h`, but could not
find the actual code. Fix: add `Enemy.cpp` to `SRCS`.

---

### Step 6 — Build and Run

Build the project. Fix any errors. Then test:

- Enemies should fall from the top of the screen at random x positions
- Shooting an enemy should make it disappear
- Enemies that reach the bottom should disappear without crashing

---

## What Your Project Looks Like Now

```
benteng/
├── main.cpp        ← director: loop, timers, spawning, wiring
├── Player.h/.cpp   ← what a Player is and does
├── Bullet.h/.cpp   ← what a Bullet is and does
├── Enemy.h/.cpp    ← what an Enemy is and does
├── HUD.h/.cpp      ← what the HUD displays
└── Makefile
```

The pattern holds: every new "thing" gets its own pair of files.
`main.cpp` grows by a few lines. Nothing else changes.

---

## Common Mistakes

### Mistake 1 — Forgetting `break` after erasing the bullet

If you erase `bullets[i]` inside the inner loop and do not `break`,
the loop will try to access `bullets[i]` again on the next iteration.
That bullet no longer exists. This causes a crash.

Always `break` out of the inner loop after a hit is detected.

### Mistake 2 — Looping forward when erasing

Erasing item `[i]` from a vector shifts all later items left by one.
If you loop forward, you skip the item that just shifted into `[i]`.
Always loop **backward** when erasing items during iteration.

### Mistake 3 — Using `int` instead of `float` for the Rectangle fields

`Rectangle` fields (`x`, `y`, `width`, `height`) are all `float`.
If you pass raw integer constants without a cast, you may get
compiler warnings about narrowing conversions.
Use `(float)` to cast, or write the literal with `.0f`: `40.0f`.

### Mistake 4 — Spawning the enemy at `y = 0` instead of `y = -height`

If the enemy spawns at `y = 0`, it appears instantly at the top edge —
which feels abrupt. Spawning at `y = -ENEMY_HEIGHT` lets it slide
smoothly into view from above.

### Mistake 5 — Not including `Bullet.h` in `Enemy.h`

`UpdateEnemies` takes a `std::vector<Bullet>&` parameter.
The compiler needs to know what `Bullet` is before it can compile that signature.
`Enemy.h` must include `Bullet.h`.

---

## Homework

Do these in order. Build and run after each one.

**Homework 1 — Get it working**
Implement everything above. Enemies should fall, bullets should destroy them,
and both should be removed correctly when they leave the screen.

**Homework 2 — Add an enemy counter to the HUD**
In `HUD.h`, add a new function:

```cpp
void DrawEnemyCount(int count);
```

In `HUD.cpp`, implement it:

```cpp
void DrawEnemyCount(int count) {
    DrawText(TextFormat("Enemies: %d", count), 10, 90, 20, RED);
}
```

Call it in `main.cpp`:

```cpp
DrawEnemyCount((int)enemies.size());
```

Watch the number go up as enemies spawn and down as you shoot them.

**Homework 3 — Make enemies speed up over time**
Right now enemies always have a random speed between 80 and 200.
Make them gradually get faster as the game goes on.

Add a `float gameTime = 0.0f;` variable in `main()`.
Every frame, add `GetFrameTime()` to it.

Then when spawning an enemy, add `gameTime * 5.0f` to the max speed:

```cpp
e.speed = (float)GetRandomValue(80, (int)(200 + gameTime * 5.0f));
```

After 10 seconds, the max speed is 250. After 20 seconds, 300.
The game gets harder the longer you survive.

**Homework 4 — Add a score (challenge)**
Add an `int score = 0;` variable in `main()`.

Every time a bullet hits an enemy (inside `UpdateEnemies`),
the score should increase by 1.

The problem: `UpdateEnemies` does not have access to `score` —
it is a local variable in `main()`.

There are two ways to solve this:

**Option A — return the kill count:**
Change `UpdateEnemies` to return an `int` — the number of enemies
killed this frame. Add it to `score` in `main()`.

```cpp
// In Enemy.h:
int UpdateEnemies(std::vector<Enemy>& enemies, std::vector<Bullet>& bullets);

// In main():
score += UpdateEnemies(enemies, bullets);
```

**Option B — pass score by reference:**
Add `int& score` as a parameter so `UpdateEnemies` can modify it directly.

```cpp
// In Enemy.h:
void UpdateEnemies(std::vector<Enemy>& enemies, std::vector<Bullet>& bullets, int& score);
```

Try both. Which feels cleaner to you?

Then display the score in the HUD:

```cpp
DrawText(TextFormat("Score: %d", score), 10, 130, 20, WHITE);
```

---

## Next

→ Chapter 06 — [Assets & Sprite Animation](06-assets-and-sprite-animation.md) (load textures, sprite sheets, and animating the engine flame)
