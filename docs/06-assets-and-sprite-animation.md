# Chapter 06 — Assets & Sprite Animation

## What You Will Learn

- What an "asset" is, and the three kinds raylib can load (textures, sounds, fonts)
- Why assets live in their own folder, and how raylib finds them (the working-directory trap)
- How to load a texture with `LoadTexture` and free it with `UnloadTexture`
- How to check that a load actually succeeded before you use the texture
- The `DrawTexture` family: `DrawTexture`, `DrawTextureEx`, `DrawTexturePro`
- What a **sprite sheet** is, and how to cut out one frame with a `source` rectangle
- How animation works: swap the frame every N seconds using a timer
- How to scale a sprite up without it looking blurry

By the end of this chapter your game still plays exactly the same — but instead of
gray and red rectangles you will have a real space ship with a flickering engine
flame, enemy ships, laser bullets, and a starfield background.

---

## The Plan

We will do it in this order:

1. Read the sprite sheets you downloaded and figure out the frame math
2. Load four textures in `main()`
3. Draw the background stretched over the whole screen
4. Replace the player square with a ship sprite
5. Animate the player ship (4 frames of engine flame!)
6. Replace the enemy rectangles with enemy ship sprites
7. Replace the bullet rectangles with bullet sprites
8. Unload everything, build, run

---

## What is an asset?

So far, every picture in your game was drawn by hand with code:
`DrawRectangle` calls created the player, the enemies, the bullets. That works,
but real games use **assets** — files that contain data, like images and sounds.

raylib can load three main kinds of asset files:

| Kind | What it is | Load function |
|---|---|---|
| **Texture** | an image (sprite, background) | `LoadTexture` |
| **Sound / Music** | audio | `LoadSound`, `LoadMusicStream` |
| **Font** | custom text style | `LoadFont` |

This chapter only covers textures. Sounds and music get their own chapter later.

The golden rule for every asset:

> **Load it once, before the game loop. Use it every frame. Unload it at the end.**

If you load inside the loop, you would load the same image hundreds of times per
second — and every load also happens on your graphics card's memory (VRAM), which
is slower and smaller than normal RAM. Load once, reuse, unload once.

---

## What is a sprite sheet?

Open one of the files you downloaded:

```
assets/spaceships/spr_spaceship_01_animation.png
```

It is **80 pixels wide and 23 pixels tall**. But it does not contain one big
ship. It contains **four ships of the same size, placed side by side**:

```
         frame 0         frame 1         frame 2         frame 3
   ┌─────────────────┬─────────────────┬─────────────────┬─────────────────┐
   │                 │                 │                 │                 │
   │     ◄ ship ►    │     ◄ ship ►    │     ◄ ship ►    │     ◄ ship ►    │
   │   small flame   │  medium flame   │   big flame     │   no flame      │
   │                 │                 │                 │                 │
   └─────────────────┴─────────────────┴─────────────────┴─────────────────┘
       20 pixels          20 pixels         20 pixels         20 pixels
```

Each one of these is called a **frame**. The four frames differ only in the
engine flame at the bottom. If you show frame 0, then 1, then 2, then 3, then
back to 0 — fast — the ship looks like its engine is alive. That is animation:
**a timer that changes which little rectangle of the image you draw.**

An image that stores several frames in a row is called a **sprite sheet**
(also: spritesheet, texture atlas, animation strip).

### Frame math

This sheet is 80 pixels wide and holds 4 frames, so each frame is:

```
80 / 4 = 20 pixels wide
23      = 23 pixels tall (frames span the full height)
```

The same math works for every sheet in the pack: frame width = sheet width
divided by the number of frames. `spr_enemy_spaceship_01_animation.png` is also
80×23 with 4 frames. If you ever buy/download a sheet with 8 frames, just divide
by 8 instead of 4.

To draw frame number `f`, you want the rectangle that starts at x = `f * 20`:

```
frame 0 → x from  0 to 20
frame 1 → x from 20 to 40
frame 2 → x from 40 to 60
frame 3 → x from 60 to 80
```

---

## Step 1 — Know your files

Your `assets/` folder should already be organized like this (we fixed the names
in the previous step — no spaces, all lowercase, snake_case):

```
assets/
├── asteroids/
├── backgrounds/
│   ├── spr_background_01.png    (256x256, a starfield)
│   ├── spr_background_02.png
│   └── spr_stars.png
├── enemy_spaceships/
│   ├── spr_enemy_spaceship_01_animation.png   (80x23, 4 frames of 20x23)
│   ├── spr_enemy_spaceship_01_static.png      (20x16, a single frame)
│   ├── spr_enemy_spaceship_02_...
│   ├── spr_enemy_spaceship_03_...
│   └── bullets/
│       ├── spr_enemy_spaceship_bullet_01.png
│       ├── spr_enemy_spaceship_bullet_02.png
│       └── spr_enemy_spaceship_bullet_03.png
├── hud/
│   └── spr_lifebar.png
├── spaceships/
│   ├── spr_spaceship_01_animation.png         (80x23, 4 frames of 20x23)
│   ├── spr_spaceship_01_static.png            (20x16, a single frame)
│   ├── spr_spaceship_02_... ... spr_spaceship_07_...
│   └── bullets/
│       ├── spr_spaceship_bullet_01.png        (4x4)
│       ├── spr_spaceship_bullet_02.png        (3x6)
│       └── spr_spaceship_bullet_03.png
└── vfx/
    ├── spr_explosion_01.png
    └── spr_explosion_02.png
```

The files named `*_animation.png` are sprite sheets. The `*_static.png` files
are single frames (we will not use them, but they are handy as a reference for
the true ship size: 20×16).

We will use exactly four files in this chapter:

| What | File | Why this one |
|---|---|---|
| Player ship | `spaceships/spr_spaceship_01_animation.png` | your ship, 4 flame frames |
| Enemy ship | `enemy_spaceships/spr_enemy_spaceship_01_animation.png` | enemy ship, 4 flame frames |
| Bullet | `spaceships/bullets/spr_spaceship_bullet_02.png` | a small vertical laser (3×6) |
| Background | `backgrounds/spr_background_01.png` | a starfield |

---

## Step 2 — How raylib finds your files

This is the #1 cause of "it works on my machine but not yours" and of sudden
crashes. Textures are loaded by a **path** — a string like
`"assets/spaceships/spr_spaceship_01_animation.png"`. But the path is relative
to the **current working directory**, not to your source file.

When you type `make run`, the Makefile runs `./build/main` from your project
folder. So the working directory is `benteng/`, and `"assets/..."` points at the
right place:

```
benteng/                      ← you are here when you run `make run`
├── assets/
│   └── spaceships/
│       └── spr_spaceship_01_animation.png
├── build/
│   └── main                  ← this binary runs
└── main.cpp
```

But if you ever run `./build/main` from inside another folder (for example
`cd build && ./main`), the path `"assets/..."` now points at `build/assets/...`
which does not exist, and the texture silently fails to load.

> **Rule: always start your game with `make run` from the project folder.**

If a texture fails, raylib does not crash — it just gives you a texture with
`id == 0`. That is why the next step teaches you to check.

---

## Step 3 — Load the textures in `main()`

First, add the loads right after `SetTargetFPS(...)`. **They must come after
`InitWindow`** — a texture lives on the GPU, and the GPU context only exists
once the window is open. Loading before `InitWindow` is a crash.

```cpp
// ---- load assets (must be AFTER InitWindow!) ----
Texture2D playerTexture = LoadTexture("assets/spaceships/spr_spaceship_01_animation.png");
Texture2D enemyTexture  = LoadTexture("assets/enemy_spaceships/spr_enemy_spaceship_01_animation.png");
Texture2D bulletTexture = LoadTexture("assets/spaceships/bullets/spr_spaceship_bullet_02.png");
Texture2D bgTexture     = LoadTexture("assets/backgrounds/spr_background_01.png");

if (playerTexture.id == 0) {
    TraceLog(LOG_WARNING, "Could not load player texture - check the path!");
}
```

### What is `Texture2D`?

`Texture2D` is a tiny struct raylib gives you. Its two most important fields:

- `id` — a number raylib uses to talk to the graphics card. `0` means "no
  texture" (the load failed).
- `width` and `height` — the image size in pixels (for our player sheet: 80 and 23).

`LoadTexture` reads the PNG file, uploads the pixels to the GPU, and returns one
of these structs. You do not have to manage the pixel data yourself — you just
keep the `Texture2D` around and hand it to draw functions.

### Why check `id == 0`?

If you typo a path, `LoadTexture` returns a texture with `id == 0`. Drawing it
is harmless (raylib draws nothing), so your game would just quietly show nothing
and you would not know why. The `if` prints a warning to the console so the
problem is visible. Do the same check for the other three textures too.

> Load once, before the `while` loop. Never call `LoadTexture` inside the loop.

---

## Step 4 — Draw the background

The background file is 256×256, but your screen is 800×450. We want it stretched
to fill everything. This is the perfect moment to meet the most important draw
function of the whole chapter:

```cpp
DrawTexturePro(
    bgTexture,                       // which texture
    {0, 0, 256, 256},                // source: which part of the image
    {0, 0, (float)screenWidth, (float)screenHeight},  // dest: where + how big
    {0, 0},                          // origin: rotation pivot
    0.0f,                            // rotation in degrees
    WHITE                            // tint (color multiplier)
);
```

Put this as the **first thing inside `BeginDrawing()`, right after
`ClearBackground`** — the background must be drawn before everything else, or
it would cover your ships.

### The big idea: source vs dest

`DrawTexturePro` is the Swiss-army knife of 2D drawing in raylib. It takes two
rectangles that mean completely different things:

- **`source`** = a rectangle **inside the image**. What part of the texture's
  pixels do we want to show? The full image here: x=0, y=0, width=256, height=256.
- **`dest`** = a rectangle **on the screen**. Where should it go, and how big
  should it appear?

raylib copies the pixels from `source` into `dest`, stretching if the sizes
differ. That one concept — pick pixels in the image, place them on screen — is
the foundation of sprite sheets: to show only one frame, you simply make
`source` smaller.

### The other parameters

- **`origin`** — the rotation pivot, measured from the dest's top-left corner.
  `{0, 0}` means "rotate around the top-left corner". We do not rotate the
  background, so `{0, 0}` and rotation `0.0f` are fine.
- **`tint`** — a color that gets multiplied into every pixel. `WHITE` means
  "don't change anything". This is how you could later tint the whole image.

Note the `(float)` casts: `Rectangle` fields are `float`, and `screenWidth` is
an `int`. Remember that rule from Chapter 02 — mixing int and float is a
compiler warning.

> **Pixel-art tip:** raylib uses "nearest neighbor" filtering by default, which
> means scaled-up pixel art stays crisp and blocky instead of blurry. If you
> ever scale up a photo-like texture and it looks soft, that is normal.

---

## Step 5 — Replace the player square with a ship

### Change the declaration in `Player.h`

```cpp
void DrawPlayer(const Player &player, Texture2D texture, int frame);
```

We changed two things:

1. `Player player` → `const Player &player` — we do not modify the player when
   drawing, so a const reference is the correct (and cheaper) way to pass it.
2. Added two parameters: the texture to draw, and which animation frame to show.

### Rewrite the body in `Player.cpp`

```cpp
void DrawPlayer(const Player &player, Texture2D texture, int frame) {
    // this sprite sheet has 4 frames of equal width, side by side
    const float frameCount  = 4.0f;
    float frameWidth  = (float)texture.width / frameCount;
    float frameHeight = (float)texture.height;

    // source = the rectangle INSIDE the image we want to show (frame number)
    Rectangle source = {frame * frameWidth, 0.0f, frameWidth, frameHeight};

    // dest = where on screen + how big (we draw it 2x bigger than the sprite)
    Rectangle dest = {
        player.position.x,
        player.position.y,
        frameWidth * 2.0f,
        frameHeight * 2.0f
    };

    Vector2 origin = {0.0f, 0.0f};

    DrawTexturePro(texture, source, dest, origin, 0.0f, WHITE);
}
```

Let's read it line by line.

### Why `frameCount = 4.0f`?

Because we looked at the file: 80 pixels wide, 4 frames → each frame is 20
pixels. The `frameWidth` line computes it for us from the actual texture, so if
you swap in a sheet with different numbers it still works — but it assumes 4
frames. If you use an 8-frame sheet, change this constant to `8.0f`. (The pack
you downloaded is all 4-frame sheets.)

### The `source` rectangle is the heart of sprite sheets

```cpp
Rectangle source = {frame * frameWidth, 0.0f, frameWidth, frameHeight};
```

- `x = frame * frameWidth` — jump to the start of the `frame`-th strip.
  Frame 0 → x=0, frame 1 → x=20, frame 2 → x=40, frame 3 → x=60.
- `y = 0` — frames start at the top of the sheet.
- `width = frameWidth`, `height = frameHeight` — the size of one frame.

Change `frame` and the ship changes. That is the whole trick of sprite sheets.

### Why `* 2.0f` in dest?

The sprite is 20×23 pixels. Your player box was 40×40. If we drew the sprite at
native size, your ship would suddenly be half the size it was before. Multiplying
by 2 gives us 40×46 — roughly the size of the old player. `dest.width` and
`dest.height` are what appear on screen; the sprite's pixels get stretched to
fit. (That is why pixel art stays crisp: the 2× scale is a clean integer.)

### Update the call in `main()`

```cpp
DrawPlayer(player, playerTexture, playerFrame);
```

`playerFrame` is the animation variable we create in the next step.

---

## Step 6 — Animate the player ship

Right now the player always shows frame 0. Time to make the flame flicker.

### Add two variables in `main()`, near the other timers

```cpp
float animTimer = 0.0f;        // counts seconds to know when to change frame
const float frameTime = 0.1f;  // show each animation frame for 0.1 seconds
```

### Add this in the UPDATE section, before `BeginDrawing`

```cpp
// pick the animation frame (0..3) based on how much time passed
animTimer += GetFrameTime();
int playerFrame = (int)(animTimer / frameTime) % 4;
```

That single line is the entire animation logic. Here is what it does, piece by
piece:

1. `animTimer += GetFrameTime();`
   Every frame, add the time since the last frame (about 1/144 of a second).
   After one second, `animTimer` is about 1.0. It just counts seconds forever.

2. `animTimer / frameTime`
   "How many frame-times have passed?" With `frameTime = 0.1`, after 0.25 s we
   get 2.5 — meaning frame 2 and a half.

3. `(int)(...)`
   Casting a `float` to `int` **truncates** (chops off the decimal part).
   2.5 becomes 2. We now have a number that goes 0, 1, 2, ... up, increasing
   once every 0.1 seconds.

4. `% 4`
   The modulo operator from Chapter 04. It wraps the number back to 0..3, so it
   never runs out: 0,1,2,3,0,1,2,3,0,1,2,3... forever.

Change `frameTime` to make the animation faster (smaller number) or slower
(bigger number). Change the `4` if your sheet has a different frame count.

### Why not just use `GetTime()`?

You could write `(int)(GetTime() / 0.1f) % 4` and skip the timer variable
entirely. It works! The reason we keep a separate `animTimer` is that later you
will want per-object timers — the enemy should not be forced to use the exact
same clock as the player, and the bullet that was just fired should start at
frame 0, not wherever the global clock happens to be. A counter that you add to
is the general pattern; `GetTime()` is a shortcut that is only good while all
animations may share one clock.

---

## Step 7 — Enemy sprites

### Change the declaration in `Enemy.h`

```cpp
void DrawEnemies(const std::vector<Enemy> &enemies, Texture2D texture);
```

Note: `std::vector<Enemy>` became `const std::vector<Enemy> &` — drawing must
not modify the list. (Your old code had it non-const; fix it while you are here.)

### Rewrite the drawing part of `Enemy.cpp`

Keep `UpdateEnemies` exactly as it is. Replace only `DrawEnemies`:

```cpp
void DrawEnemies(const std::vector<Enemy> &enemies, Texture2D texture) {
    const float frameCount  = 4.0f;
    float frameWidth  = (float)texture.width / frameCount;
    float frameHeight = (float)texture.height;

    // all enemies share the same animation clock for now
    int frame = (int)(GetTime() / 0.1f) % 4;

    Rectangle source = {frame * frameWidth, 0.0f, frameWidth, frameHeight};
    Vector2 origin = {0.0f, 0.0f};

    for (const Enemy &e : enemies) {
        Rectangle dest = {e.position.x, e.position.y, e.size.x, e.size.y};
        DrawTexturePro(texture, source, dest, origin, 0.0f, WHITE);
    }
}
```

Differences from the player:

- Enemies use the **global clock** (`GetTime()`) instead of a per-enemy timer —
  this chapter's shortcut. They all flicker in sync, which looks a little
  robotic. Fixing that is Homework 2.
- The dest uses `e.size` directly (20×23), so enemies render at **native size**.
  You must update the spawn code so the size matches the sprite:

In `main()`, in the enemy spawn block, change:

```cpp
e.size = {20, 23}; // matches the enemy sprite
```

Why does this matter? `e.size` is used for two things: how big the sprite is
drawn, **and** the collision rectangle in `UpdateEnemies`. If they disagree, you
get bullets hitting "invisible" space around the ship. Keep the drawn size and
the collision size in sync.

### Update the call in `main()`

```cpp
DrawEnemies(enemies, enemyTexture);
```

---

## Step 8 — Bullets

The bullet sprite is a single image (3×6), not a sheet — so `source` is the
whole texture.

### Change the declaration in `Bullet.h`

```cpp
void DrawBullets(const std::vector<Bullet> &bullets, Texture2D texture);
```

### Rewrite `DrawBullets` in `Bullet.cpp`

```cpp
void DrawBullets(const std::vector<Bullet> &bullets, Texture2D texture) {
    // the whole texture is one bullet (no sprite sheet here)
    Rectangle source = {0.0f, 0.0f, (float)texture.width, (float)texture.height};
    Vector2 origin = {0.0f, 0.0f};

    for (const Bullet &b : bullets) {
        // stretch the small sprite to the bullet's collision size
        Rectangle dest = {b.position.x, b.position.y, b.size.x, b.size.y};
        DrawTexturePro(texture, source, dest, origin, 0.0f, WHITE);
    }
}
```

### Update the bullet size in `main()`

In the shooting block, change:

```cpp
b.size = {6, 12}; // matches the scaled bullet sprite
```

The sprite is 3×6 pixels. We want the bullet to look like the old 6×10 one, so
we stretch it to 6×12 (a 2× scale). `b.size` is again used for both drawing and
collision — both use 6×12, so they agree.

### Update the call in `main()`

```cpp
DrawBullets(bullets, bulletTexture);
```

---

## Step 9 — Unload, build, run

### Unload the textures before `CloseWindow()`

```cpp
UnloadTexture(bgTexture);
UnloadTexture(playerTexture);
UnloadTexture(enemyTexture);
UnloadTexture(bulletTexture);
CloseWindow();
```

Every `LoadTexture` has a matching `UnloadTexture`. It frees the GPU memory.
In a tiny game it does not matter much — but the OS window is about to close
anyway, and the habit matters the day you load 200 textures.

### Build & run

```bash
make run
```

What should happen:

- A starfield fills the whole screen
- Your ship (blue, pointing up) is at the bottom, its engine flame flickering
  through 4 frames
- Enemy ships fall from the top, flames flickering in sync
- Your yellow laser bullets are now orange-and-white laser bolts
- Shooting an enemy still destroys it; everything behaves exactly as in
  Chapter 05 — only the looks changed

If the player is invisible: check the console for the `TraceLog` warning we
added — it means the path is wrong or the assets folder was moved. Run via
`make run` from the project folder.

---

## Your Full main.cpp — Chapter 06 Edition

Your `main.cpp` should now look like this:

```cpp
#include "Bullet.h"
#include "Enemy.h"
#include "HUD.h"
#include "Player.h"
#include "raylib.h"
#include <vector>

int main() {

    const int screenWidth = 800;  // x
    const int screenHeight = 450; // y

    InitWindow(screenWidth, screenHeight, "Pesawat Tempur");
    SetTargetFPS(144);

    // ---- load assets (must be AFTER InitWindow!) ----
    Texture2D playerTexture = LoadTexture("assets/spaceships/spr_spaceship_01_animation.png");
    Texture2D enemyTexture  = LoadTexture("assets/enemy_spaceships/spr_enemy_spaceship_01_animation.png");
    Texture2D bulletTexture = LoadTexture("assets/spaceships/bullets/spr_spaceship_bullet_02.png");
    Texture2D bgTexture     = LoadTexture("assets/backgrounds/spr_background_01.png");

    if (playerTexture.id == 0) {
        TraceLog(LOG_WARNING, "Could not load player texture - check the path!");
    }

    float gameTime = 0.0f;

    Player player;
    player.position = {400, 400};
    player.size = 40;
    player.speed = 200.0f;

    float animTimer = 0.0f;        // counts seconds to know when to change frame
    const float frameTime = 0.1f;  // show each animation frame for 0.1 seconds

    float fireTimer = 0.0f;
    std::vector<Bullet> bullets;

    float spawnTimer = 0.0f;
    float spawnInterval = 2.f;
    std::vector<Enemy> enemies;

    while (!WindowShouldClose()) {
        gameTime += GetFrameTime();
        spawnTimer += GetFrameTime();
        if (spawnTimer >= spawnInterval) {
            Enemy e;
            e.position.x = (float)GetRandomValue(0, 760); // random x on screen
            e.position.y = -30;                          // just above the top edge
            e.speed = (float)(GetRandomValue(80, (int)(200 + gameTime * 5.0f)));
            e.health = GetRandomValue(3, 10);
            e.size = {20, 23}; // matches the enemy sprite
            enemies.push_back(e);
            spawnTimer = 0.0f;
        }

        UpdateEnemies(enemies, bullets);

        UpdatePlayer(player, screenWidth, screenHeight);

        fireTimer += GetFrameTime();
        if (IsKeyDown(KEY_X) && fireTimer >= 0.15f) {
            Bullet b;
            b.position.x = player.position.x + player.size / 2.0f - 3.0f;
            b.position.y = player.position.y;
            b.speed = 500.0f;
            b.size = {6, 12}; // matches the scaled bullet sprite
            bullets.push_back(b);
            fireTimer = 0.f;
        }
        UpdateBullets(bullets);

        // pick the animation frame (0..3) based on how much time passed
        animTimer += GetFrameTime();
        int playerFrame = (int)(animTimer / frameTime) % 4;

        BeginDrawing();
        ClearBackground(BLACK);

        // background stretched to cover the whole screen
        DrawTexturePro(
            bgTexture,
            {0, 0, 256, 256},
            {0, 0, (float)screenWidth, (float)screenHeight},
            {0, 0},
            0.0f,
            WHITE
        );

        DrawEnemies(enemies, enemyTexture);
        DrawPlayer(player, playerTexture, playerFrame);
        DrawBullets(bullets, bulletTexture);

        DrawBulletCount((int)bullets.size());
        DrawSpeed((int)player.speed);
        DrawFPS(750, 10);
        EndDrawing();
    }

    UnloadTexture(bgTexture);
    UnloadTexture(playerTexture);
    UnloadTexture(enemyTexture);
    UnloadTexture(bulletTexture);
    CloseWindow();
    return 0;
}
```

Compare it to your Chapter 05 version and notice what actually changed:

1. Four `LoadTexture` lines + one `id == 0` check (Step 3)
2. `animTimer` and `frameTime` + the `playerFrame` line (Step 6)
3. `e.size = {20, 23};` and `b.size = {6, 12};` (Steps 7 & 8)
4. The `DrawTexturePro` background call (Step 4)
5. New arguments passed to `DrawEnemies`, `DrawPlayer`, `DrawBullets`
6. Four `UnloadTexture` lines before `CloseWindow` (Step 9)

If you still have the old experiment line `gameTime = gameTime * 5.f;` floating
around from Chapter 05, replace it with `gameTime += GetFrameTime();` — we want
seconds to *add up*, not multiply.

---

## What Your Project Looks Like Now

```
benteng/
├── assets/
│   ├── backgrounds/
│   ├── enemy_spaceships/
│   ├── hud/
│   ├── spaceships/
│   └── vfx/
├── main.cpp            ← loads textures, owns the animation timer
├── Player.h/.cpp       ← DrawPlayer now draws a sprite frame
├── Enemy.h/.cpp        ← DrawEnemies now draws sprite frames
├── Bullet.h/.cpp       ← DrawBullets now draws a sprite
├── HUD.h/.cpp
└── Makefile
```

The pattern holds: every "thing" still owns its pair of files. `main.cpp` grew
by the texture loads and one timer. Nothing else changed.

---

## Common Mistakes

### Mistake 1 — Wrong path / wrong working directory
`"assets/..."` only works if the current directory is `benteng/`. If you run
`./build/main` from somewhere else, the load fails silently and your game shows
nothing. Always `make run`, and use the `id == 0` check to make failures loud.

### Mistake 2 — Loading textures before `InitWindow`
Textures live on the GPU. The GPU context is created by `InitWindow`. Loading
earlier is a crash (or a zeroed texture). Load after `InitWindow`, unload before
`CloseWindow`.

### Mistake 3 — Loading inside the game loop
`LoadTexture` reads a file from disk and uploads to VRAM. Doing it every frame
is slow and leaks GPU memory. Load once before the `while` loop.

### Mistake 4 — Forgetting `UnloadTexture`
In this game the OS reclaims everything when the window closes, so you would not
notice. In a real project you keep loading and unloading, and every missed
`UnloadTexture` is a permanent VRAM leak. Pair every load with an unload.

### Mistake 5 — Source vs dest confusion
`source` is **inside the image**; `dest` is **on the screen**. Mixing them up is
the most common DrawTexturePro error. If your ship shows only a corner or a
blank area, check that `source` never goes beyond the texture size
(frame × frameWidth must stay inside 0..texture.width).

### Mistake 6 — Drawn size ≠ collision size
If `e.size`/`b.size` says 20×23 but the sprite is drawn at 40×46, bullets hit
an invisible box that does not match the picture. The sizes used by drawing and
by `CheckCollisionRecs` must describe the same rectangle.

### Mistake 7 — Integer math in the animation
`(int)(animTimer / frameTime) % 4` — the cast is on the **division result**, so
2.5 → 2, then % 4 wraps it. If you instead write `(int)animTimer / frameTime`,
the cast happens first and the whole animation runs 10× too fast. Also remember
`frameTime` and `animTimer` must be `float`, or the division truncates to
whole seconds.

---

## Homework

Build and run after each one.

**Homework 1 — Get it working**
Everything above. Player with flickering flame, falling enemy ships, laser
bullets, starfield.

**Homework 2 — Random start frame per enemy**
Right now all enemies animate in sync (they all use `GetTime()`). Give each
enemy its own starting point: add a `float animOffset;` field to the `Enemy`
struct, set it in `main()` to `(float)GetRandomValue(0, 100)`, and in
`DrawEnemies` compute:

```cpp
int frame = (int)((GetTime() + e.animOffset) / 0.1f) % 4;
```

Now each enemy flickers at its own random moment. Notice how one small field
fixed the "robotic" look.

**Homework 3 — Flip the enemy ship so it points down**
The sprite points up, but enemies fall down. raylib lets you flip a texture by
making the source size negative:

```cpp
Rectangle source = {frame * frameWidth, frameHeight, frameWidth, -frameHeight};
```

A negative `source.height` mirrors the image vertically — now the nose points
down and the flame is on top (which reads as "engine pushing the ship down").
Try it. (If you ever want a left-right mirror instead, negate `source.width`.)

**Homework 4 — Pick a different ship per enemy**
The pack has 3 enemy ship variants. Store the ship number on the enemy
(`int ship;`), load all three textures, and in `DrawEnemies` pass the right one:

```cpp
Texture2D tex = (e.ship == 1) ? enemyTexture1
              : (e.ship == 2) ? enemyTexture2
                              : enemyTexture3;
```

Remember the ternary operator? This is a clean use for it. Enemies that look
different are much more fun to fight.

**Homework 5 — Tile the background instead of stretching it**
Stretching 256×256 to 800×450 distorts the stars a bit. Draw the background as
a 4×2 grid of tiles instead (each tile 256×256, covering 1024×512) using a
double loop and `DrawTexturePro` per tile. The picture stays undistorted and it
teaches you the nested-loop pattern from Chapter 04 in a new place.

**Challenge — The explosion sheets**
Open `assets/vfx/spr_explosion_01.png`. Unlike the ships, its frames are not
evenly spaced — each frame is a different size with transparent padding, so the
simple `width / frameCount` math does not line up. Try to animate it anyway:
figure out the start x of each explosion frame by hand, and draw one when an
enemy dies. This is the hardest homework so far, and the payoff is the most
satisfying: enemies that explode.

---

## Next

**Chapter 07 — Sound & Music.** Make the laser pew, the explosion boom, and a
looping background track: `InitAudioDevice`, `LoadSound`, `PlaySound`,
`LoadMusicStream`, and the same load-once/use-every-frame/unload-at-the-end
discipline you learned here for textures.
