# Chapter 08 — Game States & the Standard Controller

Right now the game jumps straight into action: the window opens, enemies spawn
immediately, and the player is already flying. There is no "start", no "you
died", no "try again". This chapter gives the game a proper skeleton — a **main
menu**, a **game over** screen, and the machinery that moves between them.

And while we're at it, the keyboard handling gets fixed. Today the keys are
scattered all over the code: `KEY_ENTER` in the menu, `KEY_X` for shooting,
`KEY_UP`/`KEY_DOWN`/`KEY_LEFT`/`KEY_RIGHT` in the Player — six key names in
three files. The code speaks *keyboard*, not *intent*.

This chapter replaces that with a **Standard Controller** — a single place that
defines the whole game's control scheme:

- **Arrow keys** — move
- **X** — confirm / accept (yes, start, fire)
- **C** — cancel (no, back, quit)

Every screen speaks the same language: *arrows move, X agrees, C cancels.* And
every key is named in exactly one file.

> One chapter, one idea: **a game is a machine with states, driven by one
> controller.** The game is always in exactly one state — MENU, PLAYING, or
> GAME_OVER — and only that state decides what happens. Input never mentions a
> key; it always asks for an *action*.

---

## What You Will Learn

- What a game state is, and the Finite State Machine (FSM) idea behind it
- `enum class GameState` — a type-safe list of the states
- The **Standard Controller**: `Input.h` maps logical *actions* (Confirm,
  Cancel, MoveUp, ...) to physical keys in exactly one place
- `IsActionPressed` vs `IsActionDown` — edge trigger vs level trigger, and why
  menus need the former
- The same controller meaning different things per state: X *starts* in the
  menu, X *fires* in gameplay, X *retries* on the game over screen
- A `switch` that dispatches work per state — the FSM's "brain"
- `ResetGame()` — restarting cleanly: timers, entity list, and a brand-new
  player
- A pointer-lifetime trap: why `RemoveDead()` must **never** erase the Player

---

## The Plan

By the end of this chapter the game has three screens and one controller:

```
        ┌──────────┐   X pressed        ┌──────────┐
        │   MENU   │ ─────────────────▶ │ PLAYING  │
        └──────────┘   (confirm)        └────┬─────┘
             ▲              C = quit         │ player dies
             │ C pressed                     ▼
             │ (cancel)        ┌──────────────┐
             └─────────────────│   GAME OVER  │
                back to menu   └──────────────┘
```

Build it in this order:

1. Create `GameState.h` with the `enum class`
2. Create `Input.h` / `Input.cpp` — the Standard Controller
3. Rewire `Player` to use the controller (arrows)
4. Add the `state` field and new methods to `Game`
5. Rewrite `Run()` so each frame runs only the current state's code
6. Implement MENU — start on X, quit on C
7. Move gameplay into PLAYING — arrows move, X fires — and detect death
8. Implement GAME_OVER — retry on X, back to menu on C
9. Write `ResetGame()` — the clean-slate both transitions use
10. Patch `RemoveDead()` so the player is never erased
11. Build and test the whole cycle: menu → play → die → restart → quit

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
    GAME_OVER,  // the player died — waiting to retry
};
```

### Why `enum class`, not `enum`?

With the old-style `enum`, the names leak into the whole file — you could not
have another `MENU` anywhere. With `enum class` (C++11), the values live
*inside* the enum: you write `GameState::MENU`, not `MENU`. That is explicit,
collision-free, and impossible to mix up with a random `int` (an `int` will not
implicitly convert to a `GameState`).

> Rule: **prefer `enum class` over `enum`.** The name is longer to type and
> worth it every time.

Why a whole file for six lines? The same reason Player gets its own file
(Chapter 03): one concept, one home. A junior can open `GameState.h` and see
every screen the game has in six lines.

---

## Part 3 — The Standard Controller: `Input.h` / `Input.cpp`

### The disease first

Count how many places name a keyboard key right now:

- `Player.cpp` — `KEY_UP`, `KEY_DOWN`, `KEY_LEFT`, `KEY_RIGHT`
- `Game.cpp` — `KEY_X` (fire), and soon `KEY_ENTER` (menu) if we just added one

Six key names, three files, zero organization. Now the problems:

1. **The code says "keyboard", not "intent".** Reading `IsKeyDown(KEY_X)` in a
   game-over handler tells you nothing about *why* X matters there.
2. **Changing a key means hunting.** Want C to start instead of X? You must
   find every `KEY_X` and hope you got them all.
3. **Menu and gameplay disagree.** The menu will use ENTER, gameplay uses X —
   two different conventions for the same idea ("yes, proceed").

The fix is a layer: game code stops naming keys and starts asking for
**actions**.

### `Input.h` — the action list

Create `Input.h`:

```cpp
#pragma once

#include "raylib.h"

// The game's Standard Controller:
//   arrows move, X confirms/accepts, C cancels.
// Game code NEVER names a keyboard key — it asks for an action,
// and Input.cpp decides which physical key that action is.
enum class InputAction {
    Confirm,    // X — yes / accept / fire
    Cancel,     // C — no / back / quit
    MoveUp,     // Arrow Up
    MoveDown,   // Arrow Down
    MoveLeft,   // Arrow Left
    MoveRight,  // Arrow Right
};

// Which physical key does this action use? (the one and only mapping)
KeyboardKey KeyFor(InputAction action);

// Edge trigger — true only on the single frame the key goes down.
bool IsActionPressed(InputAction action);

// Level trigger — true every frame while the key is held.
bool IsActionDown(InputAction action);
```

### `Input.cpp` — the mapping lives in exactly one place

Create `Input.cpp`:

```cpp
#include "Input.h"

KeyboardKey KeyFor(const InputAction action) {
    switch (action) {
    case InputAction::Confirm:   return KEY_X;
    case InputAction::Cancel:    return KEY_C;
    case InputAction::MoveUp:    return KEY_UP;
    case InputAction::MoveDown:  return KEY_DOWN;
    case InputAction::MoveLeft:  return KEY_LEFT;
    case InputAction::MoveRight: return KEY_RIGHT;
    }
    return KEY_NULL; // unreachable — all cases are handled above
}

bool IsActionPressed(const InputAction action) {
    return IsKeyPressed(KeyFor(action));
}

bool IsActionDown(const InputAction action) {
    return IsKeyDown(KeyFor(action));
}
```

### Why this design wins

**1. One switch owns the entire control scheme.** Every key the game ever
reads is named inside `KeyFor` — and nowhere else. Want C to confirm instead
of X? Change two lines. That is the Chapter 06 lesson again: *single source of
truth*. The controller is the single source of truth for the controls.

**2. Edge vs level is built into the API.** You learned the difference in
Chapter 08's menu — but now it is enforced by the function names themselves:

- `IsActionPressed(...)` — "do this once" events (start, retry, confirm)
- `IsActionDown(...)` — "keep doing this" actions (move, hold to fire)

If you pick the wrong one, the *name* tells you. `KeyFor` + two tiny wrappers,
and the whole game stops asking "is this key held?" and starts asking "is
Confirm pressed?"

**3. `switch` + `enum class` is compiler-checked.** Add a fourth action later
— say `Pause` — and the compiler warns "enumeration value not handled in
switch". You cannot add an action and forget its key.

> Rule: **game logic asks for actions, never for keys.** One file maps
> actions → keys. Everything else calls `IsActionPressed` /
> `IsActionDown`.

---

## Part 4 — Rewire the Player to the controller

Open `Player.cpp`. It is the only gameplay file that reads keys directly.
Include the controller and replace every `IsKeyDown(KEY_...)` with an action:

```cpp
#include "Player.h"
#include "Config.h"
#include "Input.h"
#include "raylib.h"
#include "Sprite.h"

// ... constructor unchanged ...

void Player::Update(float deltaTime) {
    if (IsActionDown(InputAction::MoveUp) && position.y > 0)
        position.y -= speed * deltaTime;
    if (IsActionDown(InputAction::MoveDown) &&
        position.y < screenHeight - size.y)
        position.y += speed * deltaTime;
    if (IsActionDown(InputAction::MoveLeft) && position.x > 0)
        position.x -= speed * deltaTime;
    if (IsActionDown(InputAction::MoveRight) &&
        position.x < screenWidth - size.x)
        position.x += speed * deltaTime;

    animationTick += deltaTime;
}

// ... Draw and TakeDamage unchanged ...
```

`Player.h` does not change at all — the controller only touches the `.cpp`
file. Note: **a player moves with `IsActionDown`** (level trigger). Holding the
arrow keeps you moving every frame.

---

## Part 5 — Wire the state into `Game`

`Game` owns everything, so `Game` owns the state — and the exit flag. Open
`Game.h` and make these changes:

1. `#include "GameState.h"`
2. Add the per-state methods
3. Add the `state` field (starts at `MENU`) and a `running` flag

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

    GameState state = GameState::MENU;  // ← the machine starts at MENU
    bool running = true;                // ← false = the game asks to close

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

**Why a `running` flag?** The game loop is `while (!WindowShouldClose() && …
)`. To let the menu *quit on C*, a state needs a way to end the loop. The
cleanest is a flag: cancel sets `running = false`, and the loop condition
checks it. (Calling `CloseWindow()` early would work too, but the destructor
also calls it — two `CloseWindow` calls is sloppy. A flag is honest.)

Each state gets an **Update** (what it does per frame) and a **Draw** (what it
shows). MENU, PLAYING, and GAME_OVER each get a pair. That symmetry keeps the
switch in `Run()` boring and predictable.

---

## Part 6 — Rewrite `Run()`: the dispatcher

`Run()` stops doing everything and becomes a **dispatcher** — it reads the
current state and routes the work:

```cpp
void Game::Run() {
    while (!WindowShouldClose() && running) {
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

**2. `switch` on an `enum class` is checked by the compiler.** Add a fourth
state and forget to handle it here → "enumeration value not handled in
switch". The machine cannot silently forget a state.

---

## Part 7 — MENU: the title screen

### UpdateMenu — start on X, quit on C

```cpp
void Game::UpdateMenu() {
    if (IsActionPressed(InputAction::Confirm)) {
        ResetGame();          // fresh player, zeroed timers
        state = GameState::PLAYING;
    }
    if (IsActionPressed(InputAction::Cancel))
        running = false;      // C in the menu = "no thanks, quit"
}
```

### DrawMenu — centered text

```cpp
void Game::DrawMenu() const {
    const char *title = "SPACE SHOOTER";
    const char *controls = "Move: Arrows    Confirm: X    Cancel: C";
    const char *hint = "Press X to Start";

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

**The most important detail in this chapter:** `IsActionPressed`, not
`IsActionDown`.

- `IsActionDown(Confirm)` is a **level trigger** — true *every frame while X
  is held*. If you used it here, the menu would vanish in the very first frame
  while the key is still held from the last run.
- `IsActionPressed(Confirm)` is an **edge trigger** — true only on the *one
  frame* where the key goes from up to down.

> Rule: **`IsActionPressed` for "do this once" events (start, retry, confirm).
> `IsActionDown` for "keep doing this" actions (move, hold-to-fire).**

**Centering text without guessing:**

```cpp
MeasureText(text, fontSize);                 // how many pixels wide?
x = (int)((SCREEN_WIDTH - textWidth) / 2);   // left pad = right pad → centered
```

`SCREEN_WIDTH` is a `float` (from `Config.h`) and `MeasureText` returns an
`int`, so the expression is a `float`. `DrawText` wants an `int` — the
`(int)(...)` cast makes that conversion explicit instead of a narrowing
warning.

---

## Part 8 — PLAYING: the game you already wrote, plus death

This is where the code you already have lives — now speaking the controller's
language. The update block moves into a named function, `FireBullets` swaps
`IsKeyDown(KEY_X)` for `IsActionDown(Confirm)`, and a death check is added at
the end:

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

Inside `FireBullets`, one line changes:

```cpp
// before — the game names a keyboard key
if (!IsKeyDown(KEY_X) || fireTimer < 0.15f)
    return;

// after — the game asks for an action
if (!IsActionDown(InputAction::Confirm) || fireTimer < 0.15f)
    return;
```

Firing is a "keep doing this" action — hold X and bullets stream out — so it
uses `IsActionDown`.

**How does the game know the player died?** `Player::TakeDamage` already does
the work (Chapter 07):

```cpp
void Player::TakeDamage(int amount) {
    health -= amount;
    if (health <= 0)
        active = false;
}
```

So after the collision pass, `player->active == false` exactly when health hit
zero. One line flips the machine:

```cpp
if (!player->active)
    state = GameState::GAME_OVER;
```

That is the FSM transition arrow **PLAYING → GAME_OVER**, fired by the event
"the player died".

---

## Part 9 — GAME_OVER: retry on X, back to menu on C

```cpp
void Game::UpdateGameOver() {
    if (IsActionPressed(InputAction::Confirm)) {
        ResetGame();
        state = GameState::PLAYING;
    }
    if (IsActionPressed(InputAction::Cancel))
        state = GameState::MENU;   // back to the title, not quit
}

void Game::DrawGameOver() const {
    const char *title = "GAME OVER";
    const char *hint = "Press X to Play Again";
    const char *back = "Press C for Menu";

    const int titleW = MeasureText(title, 60);
    const int hintW = MeasureText(hint, 20);
    const int backW = MeasureText(back, 20);

    DrawText(title,
             (int)((SCREEN_WIDTH - titleW) / 2),
             160, 60, RED);
    DrawText(hint,
             (int)((SCREEN_WIDTH - hintW) / 2),
             270, 20, YELLOW);
    DrawText(back,
             (int)((SCREEN_WIDTH - backW) / 2),
             300, 20, LIGHTGRAY);
}
```

### The same controller, different meaning per state

Here is the whole point of the Standard Controller. One mapping (X = Confirm,
C = Cancel), interpreted differently by each screen:

| State | Confirm (X) | Cancel (C) | Movement |
|---|---|---|---|
| MENU | start the game | quit the game | — |
| PLAYING | fire a bullet | (nothing yet — reserved for pause) | arrows |
| GAME_OVER | play again | back to menu | — |

The game logic never changes meaning by checking keys — it checks *actions*,
and each state decides what an action means there. That is decoupling: the
input layer is stable, the states are free to interpret.

Notice what GAME_OVER does **not** do: it does not call `SpawnEnemies`,
`FireBullets`, `Update`, or `ResolveCollisions`. The world is frozen — the
switch only ever routes to `UpdateGameOver` and `DrawGameOver`. If an enemy
was mid-screen when you died, it stays exactly where it was.

---

## Part 10 — `ResetGame()`: the clean slate

Both "start" transitions call `ResetGame()` before entering PLAYING:

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
| new `Player` | A fresh ship with full health and position. `Player`'s constructor sets its own position and size — that is why recreating it is enough. |

Because this block was previously duplicated in the `Game` constructor, the
constructor can now simply call `ResetGame()` instead. One function, multiple
callers, zero copy-paste.

---

## Part 11 — Patch `RemoveDead()`: the dangling-pointer trap

Here is the subtle bug waiting in the shadows. The current `RemoveDead()`:

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

## Part 12 — The complete new `Game.cpp`

Here is the whole file, ready to compare against yours. Everything below
`ResetGame()` is unchanged from Chapter 07 except `FireBullets`' input line and
the `RemoveDead()` lambda:

```cpp
#include "Game.h"

#include "Bullet.h"
#include "Config.h"
#include "Enemy.h"
#include "Input.h"
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
    while (!WindowShouldClose() && running) {
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
    if (IsActionPressed(InputAction::Confirm)) {
        ResetGame();
        state = GameState::PLAYING;
    }
    if (IsActionPressed(InputAction::Cancel))
        running = false;
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
    if (IsActionPressed(InputAction::Confirm)) {
        ResetGame();
        state = GameState::PLAYING;
    }
    if (IsActionPressed(InputAction::Cancel))
        state = GameState::MENU;
}

void Game::DrawMenu() const {
    const char *title = "SPACE SHOOTER";
    const char *controls = "Move: Arrows    Confirm: X    Cancel: C";
    const char *hint = "Press X to Start";

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
    const char *hint = "Press X to Play Again";
    const char *back = "Press C for Menu";

    const int titleW = MeasureText(title, 60);
    const int hintW = MeasureText(hint, 20);
    const int backW = MeasureText(back, 20);

    DrawText(title,
             (int)((SCREEN_WIDTH - titleW) / 2),
             160, 60, RED);
    DrawText(hint,
             (int)((SCREEN_WIDTH - hintW) / 2),
             270, 20, YELLOW);
    DrawText(back,
             (int)((SCREEN_WIDTH - backW) / 2),
             300, 20, LIGHTGRAY);
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
    if (!IsActionDown(InputAction::Confirm) || fireTimer < 0.15f)
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

### Update the build

Two new files were added, so the build must know:

- **Makefile** — nothing to do. `$(wildcard src/*.cpp)` already picks up
  `src/Input.cpp` automatically.
- **CMakeLists.txt** (if you use CLion) — add the new source to the
  executable:

  ```cmake
  add_executable(main
      src/main.cpp
      src/Input.cpp       # ← add this line
      src/Player.cpp
      src/Enemy.cpp
      src/Bullet.cpp
      src/Sprite.cpp
      src/HUD.cpp
      src/Game.cpp
  )
  ```

  Forgetting this gives a linker error — "Undefined symbols:
  `IsActionPressed`" — which means the compiler saw the declaration but never
  found the code.

---

## Part 13 — Build and test the whole cycle

Build and run. Then walk the entire loop:

1. **MENU appears.** Title centered, hint yellow: "Press X to Start". No
   enemies spawn — the world is not running.
2. Press **C** → the game closes. (The `running` flag ended the loop, and the
   destructor cleaned up.) Reopen and continue.
3. Press **X** → the game starts. Arrows move you, holding X fires.
4. **Let yourself die.** The world freezes. "GAME OVER" in red.
5. Press **X** → a fresh run starts. **Check the difficulty:** enemies do not
   instantly spawn, and their speed starts from the beginning again
   (`gameTime` was reset). If they spawn instantly or start fast, you missed a
   reset in `ResetGame()`.
6. Die again, press **C** → back to the menu, not quit, not restart.
7. Press **X** twice fast. If the menu flashes by and the game starts on its
   own, you used `IsActionDown` where you should have used `IsActionPressed`.

---

## Common Mistakes

| Mistake | Why it hurts |
|---|---|
| Naming keys directly (`KEY_ENTER`, `KEY_X`) instead of actions | The key scheme is scattered across files again; remapping means hunting. The controller exists to make this impossible. |
| `IsActionDown(Confirm)` in the menu | Level trigger — the menu "passes through" in one frame while X is still held. Use `IsActionPressed`. |
| Migrating `Game` but forgetting `Player.cpp` | Half the game still names keys; two conventions at war. The whole game must speak the controller's language. |
| Updating entities in every state | The game runs in the background during the menu. Only PLAYING updates. |
| Forgetting `gameTime = 0.0f` in `ResetGame()` | Restart starts at the old difficulty. |
| Forgetting `spawnTimer`/`fireTimer` resets | First enemy or bullet arrives instantly after restart. |
| `RemoveDead()` erasing the dead Player | `player` becomes a dangling pointer → use-after-free. Skip `EntityType::PLAYER` in the lambda. |
| Forgetting a `case` in the state switches | The machine silently does nothing for that state. Let `-Wswitch` warn you (add `default: break;` if it does not). |
| Passing `(SCREEN_WIDTH - titleW) / 2` straight to `DrawText` | Narrowing `float → int` warning; the centering math is also off. Cast explicitly: `(int)(...)`. |
| Adding a new `.cpp` but not to `CMakeLists.txt` | Linker error: "Undefined symbols". Makefile users are fine; CMake users must list it. |

---

## Homework

Build and run after each one.

**Homework 1 — The full cycle.** Implement everything above: `Input.h` /
`Input.cpp`, rewired `Player`, MENU/PLAYING/GAME_OVER, `ResetGame`,
`RemoveDead` patch. Menu → play → die → retry → back to menu → quit. Commit
when the cycle feels right.

**Homework 2 — Blinking prompt.** Make the menu's "Press X to Start" blink
instead of glowing permanently:

```cpp
if ((int)(GetTime() * 2.0) % 2 == 0)
    DrawText(hint, ...);
```

`GetTime()` counts seconds; `* 2` flips the parity every half second. The
blink is a cheap "alive" signal that menus always have.

**Homework 3 — PAUSE state.** Add `PAUSED` to `GameState`. In `PLAYING`,
`Cancel` (C) toggles to `PAUSED`; in `PAUSED`, `Confirm` (X) resumes. PAUSED
draws the frozen world (reuse `DrawPlaying()`!) plus "PAUSED" text, and
updates nothing. Now you have four states — the compiler reminds you exactly
which `switch`es need a new `case`. This fills the empty "Cancel" cell in the
PLAYING row of the controller table: C finally means *cancel the run*.

**Homework 4 — Score on GAME_OVER.** Track kills and show them on the game
over screen. The trap: `ResolveCollisions()` is `const`, so it cannot touch an
`int score` member. Two honest solutions:

- remove `const` from `ResolveCollisions()` (it was already mutating the
  world through the entity pointers — the `const` was a polite fiction), or
- count kills where it's easy: in `RemoveDead()`, you already know an enemy
  was removed.

Then display `"Score: N"` under "GAME OVER" with the same centered-text trick.
Bonus: store the *best* score so restarting does not reset it.

**Homework 5 — Remap the controller.** In `Input.cpp`, change only one line:

```cpp
case InputAction::Confirm: return KEY_Z;   // X → Z, one line
```

Build and run. Every screen follows — menu starts on Z, gameplay fires on Z,
game over retries on Z. Zero other changes. *That* is the single source of
truth paying you back. Put X back when you're done.

**Homework 6 — Feel the FSM.** Open `Run()`. Read it like a story: "menu →
press X → play → die → press X → play again, or C → menu". Five minutes of
reading `Run()` tells you the entire game's flow. That readability — one file,
one switch, the whole game's skeleton — is what the FSM buys you.

---

## Next

**Chapter 09 — Sound & Music.** The skeleton is done: one controller, three
screens. Next the game gets ears — `InitAudioDevice`, `LoadSound`,
`PlaySound` for a laser and an explosion, and `LoadMusicStream` for a looping
background track, with the death sound fired exactly when the machine flips
into GAME_OVER.
