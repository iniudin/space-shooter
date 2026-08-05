# Chapter 08 — Game States: Main Menu & Game Over

Right now the game jumps straight into action: the window opens, enemies spawn
immediately, and the player is already flying. There is no "start", no "you
died", no "try again". This chapter gives the game a proper skeleton — a **main
menu**, a **game over** screen, and the machinery that moves between them.

> One chapter, one idea: **a game is a machine with states. At every moment it
> is in exactly one state — MENU, PLAYING, or GAME_OVER — and only that state
> decides what happens.**
>
> This is called a **Finite State Machine (FSM)**. Every real game — from
> Pong to Dark Souls — is built on one.

---

## What You Will Learn

- What a game state is, and the Finite State Machine (FSM) idea behind it
- `enum class GameState` — a type-safe list of the states
- A `switch` that dispatches work per state — the FSM's "brain"
- `IsKeyPressed` vs `IsKeyDown` — edge trigger vs level trigger, and why menus
  need the former
- Per-state update and per-state draw: the menu must not move enemies, and the
  game over screen must not move anything at all
- `ResetGame()` — restarting cleanly: timers, entity list, and a brand-new
  player
- A pointer-lifetime trap: why `RemoveDead()` must **never** erase the Player,
  or your `player` pointer dangles

---

## The Plan

By the end of this chapter the game has three screens and the rules to move
between them:

```
        ┌──────────┐   ENTER pressed    ┌──────────┐
        │   MENU   │ ─────────────────▶ │ PLAYING  │
        └──────────┘                    └────┬─────┘
             ▲                               │ player dies
             │                               ▼
             └── ENTER pressed ────┌──────────────┐
                 (restart)         │   GAME OVER  │
                                   └──────────────┘
```

Build it in this order:

1. Create `GameState.h` with the `enum class`
2. Add the `state` field and the new method declarations to `Game`
3. Rewrite `Run()` so each frame only runs the current state's code
4. Implement MENU — a title screen that starts the game on ENTER
5. Move the existing gameplay into PLAYING, and detect the player's death
6. Implement GAME_OVER — a screen that restarts on ENTER
7. Write `ResetGame()` — the clean-slate function both transitions use
8. Patch `RemoveDead()` so the player is never erased (the dangling-pointer
   lesson)
9. Build and test the whole cycle: menu → play → die → restart

---

## Part 1 — What is a game state?

Look at your game loop. Today it does *everything, always*:

```cpp
while (!WindowShouldClose()) {
    float deltaTime = GetFrameTime();

    SpawnEnemies(deltaTime);
    FireBullets(deltaTime);
    for (auto &thing : things)
        if (thing->active)
            thing->Update(deltaTime);
    ResolveCollisions();
    RemoveDead();
    // ... draw ...
}
```

There is no way to say "don't spawn enemies yet — the player hasn't pressed
start". There is no way to say "freeze the world — the player is dead". The
loop has no concept of *what phase the game is in*.

A **game state** fixes this. It is a named phase of the game:

- **MENU** — title screen. No enemies, no bullets. Waiting for "Start".
- **PLAYING** — the actual game. Everything you already wrote runs here.
- **GAME_OVER** — the player died. The world is frozen. Waiting for "Retry".

The rule that makes it a *machine*:

> **At every frame, the game is in exactly one state, and only that state's
> code runs.**

That's the whole idea. Three named boxes, arrows between them, and one "active
box" at any time. This is called a **Finite State Machine** — "finite" because
the list of states is fixed and small, "machine" because the rules for moving
between them are precise.

---

## Part 2 — `GameState.h`: the list of states

A state is a value from a fixed set. C++ has exactly the right tool: an
**enum** — a list of named constants.

Create `GameState.h`:

```cpp
#pragma once

// The game is always in exactly one of these states.
// Transitions happen on events: a key press, or the player dying.
enum class GameState {
    MENU,       // title screen — waiting to start
    PLAYING,    // the actual game
    GAME_OVER,  // the player died — waiting to restart
};
```

### Why `enum class`, not `enum`?

Compare:

```cpp
enum GameState { MENU, PLAYING, GAME_OVER };        // old-style enum
enum class GameState { MENU, PLAYING, GAME_OVER };  // scoped enum
```

With the old-style enum, the names leak into the whole file — you could not
have another `MENU` anywhere. With `enum class` (C++11), the values live
*inside* the enum: you write `GameState::MENU`, not `MENU`. That is explicit,
collision-free, and impossible to mix up with a random `int` (an `int` will not
implicitly convert to a `GameState`).

> Rule: **prefer `enum class` over `enum`.** The name is longer to type and
> worth it every time.

Why a whole file for six lines? The same reason Player gets its own file
(Chapter 03): one concept, one home. The enum is a concept the whole `Game`
depends on, and putting it in its own header means nothing else needs to
recompile when it changes. It also makes the state list *discoverable* — a
junior can open `GameState.h` and see every screen the game has in six lines.

---

## Part 3 — Wire the state into `Game`

`Game` owns everything, so `Game` owns the state. Open `Game.h` and make these
changes:

1. `#include "GameState.h"`
2. Add the per-state methods
3. Add the `state` field, starting at `MENU`

The new `Game.h`:

```cpp
#pragma once

#include "Entity.h"
#include "GameState.h"
#include "Player.h"
#include <memory>
#include <vector>

class Game {
public:
    Game();
    ~Game();
    void Run();

private:
    // --- the current state decides what runs ---
    void UpdateMenu();
    void UpdatePlaying(float deltaTime);
    void UpdateGameOver();
    void DrawMenu() const;
    void DrawPlaying() const;
    void DrawGameOver() const;
    void ResetGame();

    // --- gameplay helpers (only used while PLAYING) ---
    void SpawnEnemies(float deltaTime);
    void FireBullets(float deltaTime);
    void ResolveCollisions() const;
    void RemoveDead();

    GameState state = GameState::MENU;   // ← the machine starts at MENU

    Texture2D playerTexture{};
    Texture2D enemyTexture{};
    Texture2D bulletTexture{};
    Texture2D bgTexture{};

    std::vector<std::unique_ptr<Entity>> things;
    Player *player = nullptr;

    float spawnTimer = 0.0f;
    float spawnInterval = 2.0f;
    float fireTimer = 0.0f;
    float gameTime = 0.0f;
};
```

Notice the pattern: each state has an **Update** (what it does per frame) and a
**Draw** (what it shows). MENU, PLAYING, and GAME_OVER each get a pair. That
symmetry is what keeps the switch in `Run()` boring and predictable.

---

## Part 4 — Rewrite `Run()`: the dispatcher

`Run()` stops doing everything and becomes a **dispatcher** — it reads the
current state and routes the work:

```cpp
void Game::Run() {
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        // --- UPDATE: only the current state reacts ---
        switch (state) {
        case GameState::MENU:
            UpdateMenu();
            break;
        case GameState::PLAYING:
            UpdatePlaying(deltaTime);
            break;
        case GameState::GAME_OVER:
            UpdateGameOver();
            break;
        }

        // --- DRAW: background, then the current state's screen ---
        BeginDrawing();
        ClearBackground(BLACK);

        DrawTexturePro(
            bgTexture,
            {0, 0, 256, 256},
            {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT},
            {0, 0},
            0.0f,
            WHITE);

        switch (state) {
        case GameState::MENU:
            DrawMenu();
            break;
        case GameState::PLAYING:
            DrawPlaying();
            break;
        case GameState::GAME_OVER:
            DrawGameOver();
            break;
        }

        DrawFPS(750, 10);
        EndDrawing();
    }
}
```

Two things to notice:

**1. The `switch` is the FSM's brain.** Each frame, the same question: *what
state are we in?* The answer picks exactly one function. No `if` chains, no
state checks scattered across the file — every transition decision lives in
one obvious place.

**2. `switch` on an `enum class` is checked by the compiler.** If you ever add
a fourth state to `GameState` and forget to handle it here, the compiler warns
"enumeration value not handled in switch". The machine cannot silently forget
a state. (If your compiler does not warn, add a `default: break;` to silence
the case where nothing matches.)

---

## Part 5 — MENU: the title screen

### UpdateMenu — leave on ENTER

```cpp
void Game::UpdateMenu() {
    if (IsKeyPressed(KEY_ENTER)) {
        ResetGame();          // fresh player, zeroed timers
        state = GameState::PLAYING;
    }
}
```

**The most important detail in this chapter:** `IsKeyPressed`, not
`IsKeyDown`.

- `IsKeyDown(KEY_ENTER)` is a **level trigger** — it is `true` *every frame
  while the key is held*. If you used it here, the menu would vanish in the
  very first frame, because the key is usually still held from the last run.
- `IsKeyPressed(KEY_ENTER)` is an **edge trigger** — it is `true` only on the
  *one frame* where the key goes from up to down.

> Rule: **`IsKeyPressed` for "do this once" events (menus, jumps, starts).
> `IsKeyDown` for "keep doing this" actions (movement, firing).**

### DrawMenu — centered text

```cpp
void Game::DrawMenu() const {
    const char *title = "SPACE SHOOTER";
    const char *controls = "Move: Arrow Keys    Shoot: X";
    const char *hint = "Press ENTER to Start";

    const int titleW = MeasureText(title, 60);
    const int controlsW = MeasureText(controls, 20);
    const int hintW = MeasureText(hint, 20);

    DrawText(title,
             (int)((SCREEN_WIDTH - titleW) / 2),
             140, 60, WHITE);
    DrawText(controls,
             (int)((SCREEN_WIDTH - controlsW) / 2),
             250, 20, LIGHTGRAY);
    DrawText(hint,
             (int)((SCREEN_WIDTH - hintW) / 2),
             290, 20, YELLOW);
}
```

**Centering text without guessing:**

```cpp
MeasureText(text, fontSize);   // how many pixels wide is this text?

x = (SCREEN_WIDTH - textWidth) / 2;   // left pad = right pad → centered
```

`SCREEN_WIDTH` is a `float` (from `Config.h`) and `MeasureText` returns an
`int`, so the expression is a `float`. `DrawText` wants an `int` — the
`(int)(...)` cast makes that conversion explicit instead of a narrowing
warning. (Your compiler should warn about the implicit conversion; the cast is
you saying "yes, I know, truncate it".)

---

## Part 6 — PLAYING: the game you already wrote, plus death

This is where the code you already have lives. It is the same five steps, just
moved into a named function — plus the death check at the end:

```cpp
void Game::UpdatePlaying(float deltaTime) {
    gameTime += deltaTime;      // only counts while actually playing

    SpawnEnemies(deltaTime);
    FireBullets(deltaTime);

    for (auto &thing : things) {
        if (thing->active)
            thing->Update(deltaTime);
    }

    ResolveCollisions();
    RemoveDead();

    if (!player->active)
        state = GameState::GAME_OVER;
}
```

And the drawing, also moved into its own function:

```cpp
void Game::DrawPlaying() const {
    for (auto &thing : things) {
        if (thing->active)
            thing->Draw();
    }
}
```

**How does the game know the player died?**

`Player::TakeDamage` already does the work (Chapter 07):

```cpp
void Player::TakeDamage(int amount) {
    health -= amount;
    if (health <= 0)
        active = false;
}
```

So after the collision pass, `player->active` is `false` exactly when health
hit zero. One line checks it, one line flips the machine:

```cpp
if (!player->active)
    state = GameState::GAME_OVER;
```

This is the FSM transition arrow **PLAYING → GAME_OVER**, and the event that
fires it is "the player died".

---

## Part 7 — GAME_OVER: the "you died" screen

Symmetric with MENU, except the text:

```cpp
void Game::UpdateGameOver() {
    if (IsKeyPressed(KEY_ENTER)) {
        ResetGame();
        state = GameState::PLAYING;
    }
}

void Game::DrawGameOver() const {
    const char *title = "GAME OVER";
    const char *hint = "Press ENTER to Play Again";

    const int titleW = MeasureText(title, 60);
    const int hintW = MeasureText(hint, 20);

    DrawText(title,
             (int)((SCREEN_WIDTH - titleW) / 2),
             160, 60, RED);
    DrawText(hint,
             (int)((SCREEN_WIDTH - hintW) / 2),
             280, 20, YELLOW);
}
```

Notice what GAME_OVER does **not** do: it does not call `SpawnEnemies`,
`FireBullets`, `Update`, or `ResolveCollisions`. The world is frozen — the
switch only ever routes to `UpdateGameOver` and `DrawGameOver`. If an enemy was
mid-screen when you died, it stays exactly where it was. *That* is the FSM
doing its job.

---

## Part 8 — `ResetGame()`: the clean slate

Both transitions call `ResetGame()` before entering PLAYING. It is the "new
game" button:

```cpp
void Game::ResetGame() {
    things.clear(); // delete every entity from the previous run

    spawnTimer = 0.0f;
    fireTimer = 0.0f;
    gameTime = 0.0f;

    auto p = std::make_unique<Player>(
        playerTexture, SCREEN_WIDTH, SCREEN_HEIGHT, 200.0f);
    player = p.get();
    things.push_back(std::move(p));
}
```

What it resets, and why each line matters:

| Line | What it prevents |
|---|---|
| `things.clear()` | Leftover enemies/bullets from the last run would still be flying when you respawn. `clear()` destroys them all (the `unique_ptr`s delete them). |
| `spawnTimer = 0.0f` | Without it, the first enemy of the new run could spawn instantly. |
| `fireTimer = 0.0f` | Same idea for bullets. |
| `gameTime = 0.0f` | Without it, enemies keep spawning at the *old* difficulty — the "speed grows with time" formula in `SpawnEnemies` would start mid-game. |
| new `Player` | A fresh ship with full health and position. Note `Player`'s constructor sets its own position and size — that is why recreating it is enough. |

Because the constructor used to create the player was identical to this block,
the `Game` constructor can now simply call `ResetGame()` instead of duplicating
it. One function, two callers, zero copy-paste.

---

## Part 9 — Patch `RemoveDead()`: the dangling-pointer trap

Here is the subtle bug waiting in the shadows. Look at the current
`RemoveDead()`:

```cpp
void Game::RemoveDead() {
    things.erase(
        std::remove_if(
            things.begin(),
            things.end(),
            [](const std::unique_ptr<Entity> &e) { return !e->active; }),
        things.end());
}
```

The player just died → `player->active == false` → this lambda says "dead,
erase it!" → the `unique_ptr` deletes the Player object → **`Game::player` is
now a dangling pointer** — it points at memory that no longer exists.

It will not crash immediately (the memory is untouched, for now), but the next
code that reads `player->position` — say, `FireBullets`, or any future feature
— reads garbage. Use-after-free is exactly the class of bug that appears
randomly and is a nightmare to find.

The fix: **the player is permanent. It never gets removed, dead or alive.**
It is only *hidden* (not drawn, not updated) while inactive.

```cpp
void Game::RemoveDead() {
    things.erase(
        std::remove_if(
            things.begin(),
            things.end(),
            [](const std::unique_ptr<Entity> &e) {
                // Enemies and bullets die and get removed.
                // The Player never does — `player` must stay valid.
                return !e->active && e->type() != EntityType::PLAYER;
            }),
        things.end());
}
```

> Rule: **if you keep a raw pointer to an object, make sure that object can
> never be deleted out from under you.** Either it outlives the pointer's uses,
> or you remove the pointer when the object dies.

After death, the inactive Player stays in `things` until `ResetGame()`
`clear()`s it — and `ResetGame()` immediately creates the replacement before
`player` is used again. The pointer is valid at every moment it is read.

---

## Part 10 — The complete new `Game.cpp`

Here is the whole file, ready to compare against yours. Everything below
`ResetGame()` is unchanged from Chapter 07 except the `RemoveDead()` lambda:

```cpp
#include "Game.h"

#include "Bullet.h"
#include "Config.h"
#include "Enemy.h"
#include "Sprite.h"

Game::Game() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Space Shooter");
    SetTargetFPS(144);

    playerTexture = LoadTexture(
        "../assets/spaceships/spr_spaceship_01_animation.png");
    enemyTexture = LoadTexture(
        "../assets/enemy_spaceships/spr_enemy_spaceship_01_animation.png");
    bulletTexture = LoadTexture(
        "../assets/spaceships/bullets/spr_spaceship_bullet_02.png");
    bgTexture = LoadTexture(
        "../assets/backgrounds/spr_background_01.png");

    if (playerTexture.id == 0)
        TraceLog(LOG_WARNING, "Could not load player texture");

    ResetGame(); // fresh player, ready to play
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

        switch (state) {
        case GameState::MENU:
            UpdateMenu();
            break;
        case GameState::PLAYING:
            UpdatePlaying(deltaTime);
            break;
        case GameState::GAME_OVER:
            UpdateGameOver();
            break;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        DrawTexturePro(
            bgTexture,
            {0, 0, 256, 256},
            {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT},
            {0, 0},
            0.0f,
            WHITE);

        switch (state) {
        case GameState::MENU:
            DrawMenu();
            break;
        case GameState::PLAYING:
            DrawPlaying();
            break;
        case GameState::GAME_OVER:
            DrawGameOver();
            break;
        }

        DrawFPS(750, 10);
        EndDrawing();
    }
}

void Game::UpdateMenu() {
    if (IsKeyPressed(KEY_ENTER)) {
        ResetGame();
        state = GameState::PLAYING;
    }
}

void Game::UpdatePlaying(float deltaTime) {
    gameTime += deltaTime;

    SpawnEnemies(deltaTime);
    FireBullets(deltaTime);

    for (auto &thing : things) {
        if (thing->active)
            thing->Update(deltaTime);
    }

    ResolveCollisions();
    RemoveDead();

    if (!player->active)
        state = GameState::GAME_OVER;
}

void Game::UpdateGameOver() {
    if (IsKeyPressed(KEY_ENTER)) {
        ResetGame();
        state = GameState::PLAYING;
    }
}

void Game::DrawMenu() const {
    const char *title = "SPACE SHOOTER";
    const char *controls = "Move: Arrow Keys    Shoot: X";
    const char *hint = "Press ENTER to Start";

    const int titleW = MeasureText(title, 60);
    const int controlsW = MeasureText(controls, 20);
    const int hintW = MeasureText(hint, 20);

    DrawText(title,
             (int)((SCREEN_WIDTH - titleW) / 2),
             140, 60, WHITE);
    DrawText(controls,
             (int)((SCREEN_WIDTH - controlsW) / 2),
             250, 20, LIGHTGRAY);
    DrawText(hint,
             (int)((SCREEN_WIDTH - hintW) / 2),
             290, 20, YELLOW);
}

void Game::DrawPlaying() const {
    for (auto &thing : things) {
        if (thing->active)
            thing->Draw();
    }
}

void Game::DrawGameOver() const {
    const char *title = "GAME OVER";
    const char *hint = "Press ENTER to Play Again";

    const int titleW = MeasureText(title, 60);
    const int hintW = MeasureText(hint, 20);

    DrawText(title,
             (int)((SCREEN_WIDTH - titleW) / 2),
             160, 60, RED);
    DrawText(hint,
             (int)((SCREEN_WIDTH - hintW) / 2),
             280, 20, YELLOW);
}

void Game::ResetGame() {
    things.clear(); // delete every entity from the previous run

    spawnTimer = 0.0f;
    fireTimer = 0.0f;
    gameTime = 0.0f;

    auto p = std::make_unique<Player>(
        playerTexture, SCREEN_WIDTH, SCREEN_HEIGHT, 200.0f);
    player = p.get();
    things.push_back(std::move(p));
}

void Game::SpawnEnemies(float deltaTime) {
    spawnTimer += deltaTime;
    if (spawnTimer < spawnInterval)
        return;
    spawnTimer = 0.0f;

    auto x = static_cast<float>(GetRandomValue(0, 760));
    auto speed = static_cast<float>(GetRandomValue(
        80,
        static_cast<int>(200 + gameTime * 5.0f)));
    int health = GetRandomValue(2, 5);

    things.push_back(std::make_unique<Enemy>(
        enemyTexture, Vector2{.x = x, .y = -30.0f}, health, speed));
}

void Game::FireBullets(float deltaTime) {
    fireTimer += deltaTime;
    if (!IsKeyDown(KEY_X) || fireTimer < 0.15f)
        return;
    fireTimer = 0.0f;

    float bulletX = player->position.x + player->size.x / 2.0f;
    Vector2 bulletSize = GetFrameSize(bulletTexture, 1.0f, SPRITE_SCALE);
    bulletX -= bulletSize.x / 2.0f;

    things.push_back(std::make_unique<Bullet>(
        bulletTexture,
        Vector2{.x = bulletX, .y = player->position.y},
        500.0f));
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

            if (a->type() == EntityType::BULLET &&
                b->type() == EntityType::ENEMY) {
                const auto &bullet = dynamic_cast<Bullet &>(*a);
                auto &enemy = dynamic_cast<Enemy &>(*b);
                enemy.TakeDamage(bullet.damage);
                a->active = false;
            }

            if (a->type() == EntityType::ENEMY &&
                b->type() == EntityType::PLAYER) {
                auto &hitPlayer = dynamic_cast<Player &>(*b);
                hitPlayer.TakeDamage(1);
                a->active = false;
            }
        }
    }
}

void Game::RemoveDead() {
    things.erase(
        std::remove_if(
            things.begin(),
            things.end(),
            [](const std::unique_ptr<Entity> &e) {
                return !e->active && e->type() != EntityType::PLAYER;
            }),
        things.end());
}
```

---

## Part 11 — Build and test the whole cycle

Build and run. Then walk the entire loop:

1. **MENU appears.** The title is centered, the hint is yellow. No enemies
   spawn — the world is not running.
2. Press **ENTER** → the game starts. Enemies fall, you shoot, you fly.
3. **Let yourself die** (or spawn a few heavy hits — enemies deal 1 damage
   each, you have 3 health). The world freezes. "GAME OVER" appears in red.
4. Press **ENTER** → a fresh run starts. **Check the difficulty:** enemies do
   not instantly spawn, and their speed starts from the beginning again
   (`gameTime` was reset). If they spawn instantly or start fast, you missed a
   reset in `ResetGame()`.
5. Press **ENTER** twice fast. If the menu flashes by and the game starts on
   its own, you used `IsKeyDown` somewhere you should have used
   `IsKeyPressed`.

---

## Common Mistakes

| Mistake | Why it hurts |
|---|---|
| `IsKeyDown(KEY_ENTER)` in the menu | Level trigger — the menu "passes through" in one frame while the key is still held. Use `IsKeyPressed`. |
| Updating entities in every state | The game runs in the background during the menu. Only PLAYING updates. |
| Forgetting `gameTime = 0.0f` in `ResetGame()` | Restart starts at the old difficulty. |
| Forgetting `spawnTimer`/`fireTimer` resets | First enemy or bullet arrives instantly after restart. |
| `RemoveDead()` erasing the dead Player | `player` becomes a dangling pointer → use-after-free. Skip `EntityType::PLAYER` in the lambda. |
| Forgetting a `case` in the state switches | The machine silently does nothing for that state. Let `-Wswitch` warn you (add `default: break;` if it does not). |
| Passing `(SCREEN_WIDTH - titleW) / 2` straight to `DrawText` | Narrowing `float → int` warning; the centering math is also off by one. Cast explicitly: `(int)(...)`. |
| Drawing `player` directly in GAME_OVER | The dead player is inactive — the entity loop already skips it. Never `Draw()` it manually; render only what's active. |

---

## Homework

Build and run after each one.

**Homework 1 — The full cycle.** Implement everything above. Menu → play →
die → restart, with clean difficulty reset each time. Commit when the cycle
feels right.

**Homework 2 — Blinking prompt.** Make the "Press ENTER" hint blink instead of
glowing permanently:

```cpp
if ((int)(GetTime() * 2.0) % 2 == 0)
    DrawText(hint, ...);
```

`GetTime()` counts seconds; `* 2` flips the parity every half second. The
blink is a cheap "alive" signal that menus always have.

**Homework 3 — PAUSE state.** Add `PAUSED` to `GameState`. The `P` key toggles
PLAYING ↔ PAUSED. PAUSED draws the frozen world (you can reuse
`DrawPlaying()`!) plus "PAUSED" text, and updates nothing. Now you have four
states, and the compiler will remind you exactly which `switch`es need a new
`case` — feel how the FSM pays you back.

**Homework 4 — Score on GAME_OVER.** Track kills and show them on the game
over screen. The trap: `ResolveCollisions()` is `const`, so it cannot touch an
`int score` member. Two honest solutions:

- remove `const` from `ResolveCollisions()` (it was already mutating the
  world through the entity pointers — the `const` was a polite fiction), or
- count kills where it's easy: in `RemoveDead()`, you already know an enemy
  was removed.

Then display `"Score: N"` under "GAME OVER" using the same centered-text trick.
Bonus: store the *best* score so restarting does not reset it.

**Homework 5 — Feel the FSM.** After everything, open `Run()`. Read it like a
story: "menu → press start → play → die → press start → play". Five minutes of
reading `Run()` tells you the entire game's flow. That readability — one file,
one switch, the whole game's skeleton — is what the FSM buys you.

---

## Next

**Chapter 09 — Sound & Music.** The skeleton is done: menu, play, game over.
Next the game gets ears — `InitAudioDevice`, `LoadSound`, `PlaySound` for a
laser and an explosion, and `LoadMusicStream` for a looping background track,
with the death sound fired exactly when the machine flips into GAME_OVER.
