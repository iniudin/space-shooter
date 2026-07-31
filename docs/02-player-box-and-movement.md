# Chapter 02 — Player Box & Keyboard Movement

## What You Will Learn

- What a struct is and why you need one
- How to draw a rectangle on screen
- How keyboard input works in raylib
- What delta time is and why it matters
- How to write clean modular functions
- How to wire everything together in `main()`

---

## The Plan

By the end of this chapter your game will:

1. Show a colored box on screen
2. Move the box with WASD keys
3. Be structured so adding new features later is easy

You will build it in 4 steps:
1. Create a `Player` struct (the data)
2. Write `DrawPlayer()` (the appearance)
3. Write `UpdatePlayer()` (the behavior)
4. Wire it all in `main()`

---

## Step 1 — The Player Struct

### What is a struct?

A struct is a way to group related data together under one name.

Without a struct, if you want to track your player you might write:

```cpp
float playerX = 400;
float playerY = 200;
float playerSpeed = 200;
int playerSize = 40;
```

That is 4 separate variables. Now imagine also having an enemy,
bullets, and power-ups — each with their own X, Y, speed, size.
You would have dozens of unrelated variables floating around.

A struct bundles them:

```cpp
struct Player {
    Vector2 position;
    float speed;
    int size;
};
```

Now `Player` is a new type you invented. You can create one like this:

```cpp
Player player;
player.position = { 400, 200 };
player.speed = 200.0f;
player.size = 40;
```

And access its data anywhere using a dot:

```cpp
player.position.x   // the x coordinate
player.speed        // the speed
```

### What is Vector2?

`Vector2` is a type that raylib gives you. It holds two floats: `x` and `y`.
It is just a convenient way to store a 2D position without writing
two separate variables.

```cpp
Vector2 position;
position.x = 400;  // horizontal (left = 0, right = 800)
position.y = 200;  // vertical (top = 0, bottom = 450)
```

### What is `f` after a number?

```cpp
player.speed = 200.0f;
```

In C++, numbers with a decimal point are `double` by default (high precision).
Adding `f` makes it a `float` (lower precision, but faster and smaller).
Since `speed` is declared as `float`, you should use `f` to avoid
a type mismatch warning from the compiler.

### Where to put the struct?

Put it **above** `main()`. Functions need to know about `Player`
before they can use it, and C++ reads files top to bottom.

```cpp
#include "raylib.h"

struct Player {        // ← here, above main
    Vector2 position;
    float speed;
    int size;
};

int main() { ... }
```

---

## Step 2 — DrawPlayer()

This function's only job is to draw the player on screen.
It does not move anything. It does not read input. Just draw.

```cpp
void DrawPlayer(Player player)
{
    DrawRectangle(
        (int)player.position.x,
        (int)player.position.y,
        player.size,
        player.size,
        DARKBLUE
    );
}
```

### Breaking it down

`void` means this function does not return any value.
It just does its job and finishes.

`DrawRectangle` is a raylib function. It takes:
- `x` — left edge of the rectangle, in pixels from the left of the window
- `y` — top edge of the rectangle, in pixels from the top of the window
- `width` — how wide in pixels
- `height` — how tall in pixels
- `color` — a raylib color constant like `DARKBLUE`, `RED`, `GREEN`

### What is `(int)` ?

`DrawRectangle` expects whole numbers (integers) for x and y.
But `position.x` is a `float` (has decimals like 400.73).
The `(int)` is a cast — it converts the float to an integer by
cutting off the decimal part. `400.73` becomes `400`.

This is needed to avoid a compiler warning about mismatched types.

### Why pass `Player player` without `&` here?

When you pass `Player player`, the function gets its own copy.
Changes inside the function do not affect the original.
That is fine here — drawing does not need to change anything.

---

## Step 3 — UpdatePlayer()

This function reads keyboard input and moves the player.

```cpp
void UpdatePlayer(Player& player)
{
    float delta = GetFrameTime();

    if (IsKeyDown(KEY_W)) player.position.y -= player.speed * delta;
    if (IsKeyDown(KEY_S)) player.position.y += player.speed * delta;
    if (IsKeyDown(KEY_A)) player.position.x -= player.speed * delta;
    if (IsKeyDown(KEY_D)) player.position.x += player.speed * delta;
}
```

### Breaking it down

#### `Player&` — passing by reference

The `&` means "reference". Instead of getting a copy,
the function works directly on the real player.

```cpp
void UpdatePlayer(Player player)   // copy — changes are LOST after function ends
void UpdatePlayer(Player& player)  // reference — changes STICK
```

You must use `&` here. If you do not, the player moves inside the
function but snaps back to its original position every frame.

#### `GetFrameTime()`

Returns the number of seconds that passed since the last frame.
At 90 FPS, each frame takes about 0.011 seconds.
This is called **delta time**.

#### Why multiply by delta?

```cpp
// Without delta:
player.position.x += player.speed;
// At 90 FPS: moves 200 * 90 = 18,000 pixels per second
// At 60 FPS: moves 200 * 60 = 12,000 pixels per second
// The speed changes depending on the frame rate!

// With delta:
player.position.x += player.speed * delta;
// At 90 FPS: moves 200 * 0.011 * 90 = 200 pixels per second
// At 60 FPS: moves 200 * 0.016 * 60 = 200 pixels per second
// Always the same speed, regardless of frame rate.
```

Speed becomes "pixels per second" instead of "pixels per frame".
Always use delta time for movement. This is not optional.

#### `IsKeyDown` vs `IsKeyPressed`

| Function | When it returns true |
|---|---|
| `IsKeyDown(key)` | Every frame the key is held down |
| `IsKeyPressed(key)` | Only the first frame the key is pressed |

For movement you want `IsKeyDown` — you want the player to keep
moving while the key is held. `IsKeyPressed` would only move
the player one tiny step per key press.

#### The coordinate system

In raylib (and most 2D graphics):
- `x` increases going **right**
- `y` increases going **down** (opposite of math class)

```
(0,0) ─────────────── x →
  │
  │
  y
  ↓
```

So:
- Moving **up** means `y -= ...` (decreasing y)
- Moving **down** means `y += ...` (increasing y)
- Moving **left** means `x -= ...`
- Moving **right** means `x += ...`

---

## Step 4 — Wire it in main()

```cpp
int main()
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Pesawat Tempur");
    SetTargetFPS(90);

    Player player;
    player.position = { 400, 200 };
    player.speed = 200.0f;
    player.size = 40;

    while (!WindowShouldClose())
    {
        // --- UPDATE ---
        UpdatePlayer(player);

        // --- DRAW ---
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawPlayer(player);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
```

`main()` does not know how the player moves or how it looks.
It only knows: "update the player, then draw the player."
That is its job. Keep `main()` dumb — the details live elsewhere.

---

## The Full Picture

```
struct Player           holds data
UpdatePlayer(Player&)   changes data based on input
DrawPlayer(Player)      reads data and draws it
main()                  creates player, runs the loop
```

Data flows in one direction:
```
input → UpdatePlayer → player.position → DrawPlayer → screen
```

---

## Homework

Do these in order. Build and run after each one.

**Homework 1 — Just draw the box**
Add the `Player` struct and `DrawPlayer()` function.
Create a player in `main()` and call `DrawPlayer()` in the draw step.
Do not add movement yet. Just make the box appear.

**Homework 2 — Add movement**
Add `UpdatePlayer()` and call it in the update step.
Make sure the player moves with WASD.

**Homework 3 — Experiment with speed**
Change `player.speed` to `50.0f`. How does it feel?
Change it to `500.0f`. How does it feel?
Pick a speed you like and keep it.

**Homework 4 — Change the color**
Change `DARKBLUE` in `DrawPlayer()` to another color.
Available colors: `RED`, `GREEN`, `BLUE`, `YELLOW`, `ORANGE`,
`PURPLE`, `MAROON`, `LIME`, `GOLD`, `PINK`, `GRAY`, `BLACK`.

**Homework 5 (challenge) — Stop at the edges**
The player can walk off screen right now. Add code inside
`UpdatePlayer()` so the player cannot go past the window edges.
The window is 800 wide and 450 tall.
Hint: after moving, check if the position went out of bounds,
and if so, clamp it back.

---

## Next

→ [Chapter 03 — Splitting Into Multiple Files](03-splitting-into-files.md)
