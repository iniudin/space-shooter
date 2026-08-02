# Chapter 06 — Sprites, Animation & Sizes That Can't Lie

Until now everything was drawn by hand: `DrawRectangle`. This chapter replaces
those rectangles with real sprites — and, while we're at it, kills the class of
bug where the *drawing* says one size and the *collision* says another.

That second half is the actual point. Textures and animation are the means;
**"the game object's size must come from the texture, not from your memory"**
is the lesson.

> One chapter, one idea: **the texture is the single source of truth for size.
> Derive from it. Never re-type it.**

---

## What You Will Learn

- Load a texture once, use it every frame, unload it once (the working-directory
  trap included)
- Sprite sheets and the frame math: `frameWidth = width / frameCount`
- `DrawTexturePro`: `source` (pixels inside the image) vs `dest` (rectangle on
  screen)
- Animation = a timer picking a frame
- `GetFrameSize()` — a 3-line helper that makes every entity size come from the
  texture, so **drawing and collision can never disagree again**

---

## Part 1 — Load, use, unload

Textures are files (PNG) that must be moved to the GPU. The golden rule:

> Load it once, **after** `InitWindow`, **before** the loop. Unload it once,
> before `CloseWindow`.

```cpp
InitWindow(screenWidth, screenHeight, "Pesawat Tempur");

Texture2D playerTexture = LoadTexture("assets/spaceships/spr_spaceship_01_animation.png");
Texture2D enemyTexture  = LoadTexture("assets/enemy_spaceships/spr_enemy_spaceship_01_animation.png");
Texture2D bulletTexture = LoadTexture("assets/spaceships/bullets/spr_spaceship_bullet_02.png");
Texture2D bgTexture     = LoadTexture("assets/backgrounds/spr_background_01.png");

if (playerTexture.id == 0)
    TraceLog(LOG_WARNING, "Could not load player texture - check the path!");
```

Two traps:

- **Paths are relative to the working directory**, not your source file. Run
  via `make run` from `space-shooter/` — never `cd build && ./main`.
- **`id == 0` means the load failed.** raylib won't crash, it just draws
  nothing. Check the id, or you'll debug an invisible ship.
- **Inside the loop is forbidden.** Loading every frame is disk+VRAM churn.

`Texture2D` gives you what you need: `width`, `height`, `id`.

---

## Part 2 — Sprite sheets & frame math

`playerTexture` is **80×23** pixels, but it holds **four ships side by side**
(the pack's flame frames). A sheet like this is a sprite sheet.

```
  frame 0 | frame 1 | frame 2 | frame 3
   20px   |  20px   |  20px   |  20px
```

So one frame is:

```
frameWidth  = texture.width  / 4   // 80 / 4 = 20
frameHeight = texture.height      // 23
```

And frame number `f` starts at x = `f * frameWidth`. That's the *entire* math
of sprite sheets. Every sheet in this pack is a 4-frame strip; if you ever get
an 8-frame sheet, the `4` becomes `8`.

---

## Part 3 — Source vs dest

`DrawTexturePro` is the one function you need for all of this:

```cpp
DrawTexturePro(texture,   // which image
               source,    // RECTANGLE INSIDE THE IMAGE  (what pixels to show)
               dest,      // RECTANGLE ON THE SCREEN     (where + how big)
               {0, 0},    // rotation pivot
               0.0f,      // rotation
               WHITE);    // tint
```

- **source** picks pixels in the image. For frame `f`: `{f * frameWidth, 0,
  frameWidth, frameHeight}`.
- **dest** places them on screen at any size — raylib stretches. To draw the
  sprite 2× bigger, make dest 2× the frame size.
- **origin** is the rotation pivot in dest coordinates; `{0,0}` = top-left.

If your ship shows a corner or blank area, `source` went outside the image —
that's the #1 `DrawTexturePro` error.

---

## Part 4 — Animation

Animation is just "change the frame every N seconds". You already own all the
pieces:

```cpp
float animTimer = 0.0f;         // in main(), near the other timers
const float frameTime = 0.1f;   // 10 frames per second

// each frame, before drawing:
animTimer += GetFrameTime();
int playerFrame = (int)(animTimer / frameTime) % 4;   // 0,1,2,3,0,1,2,3...
```

- `animTimer / frameTime` → how many frame-times have passed (2.5, 3.0, ...)
- `(int)` truncates → 2
- `% 4` wraps into 0..3 forever

Keep a per-object timer rather than the global `GetTime()` — later you'll want
each enemy to start at its own frame, and a fresh bullet to start at frame 0.

Now `DrawPlayer(player, playerTexture, playerFrame)` draws one flame frame.

---

## Part 5 — The point: sizes must come from the texture

Here is the code smell you have had since Chapter 05, and the reason this
chapter exists:

```cpp
player.size = 40;      // int! one number for a 2D thing
e.size = {20, 23};     // "matches the enemy sprite" — TODAY
b.size = {6, 12};      // where did 6x12 come from? the sprite is 3x6, scaled 2x
```

Read the enemy line carefully. The texture is *right there* — `enemyTexture`
knows it is 80×23. You typed `{20, 23}` anyway. That's **duplication**: the
same fact in two places, and two copies always drift apart. Worse, the
collision box is invisible, so when it drifts you won't notice until bullets
fly through ship wings.

And it has *already* drifted. In your current code:

- `DrawEnemies` draws the enemy at `frameWidth * 2` = **40×46**
- `e.size = {20, 23}` → collision box is **20×23**

The enemy looks 40×46 and gets hit like a 20×23. The visible wings are ghosts.
Shoot an enemy at the wing tip and watch the bullet pass through.

The rule:

> **If a fact already exists somewhere (the texture knows its size), read it
> from there. Never write it a second time.**

### The fix: `Sprite.h` / `Sprite.cpp`

One 3-line function gives the frame math a single home:

```cpp
// Sprite.h
#pragma once
#include "raylib.h"

// On-screen size of ONE frame of a sheet, scaled.
// frameCount = 1 for a single image (bullet); > 1 for a sheet.
Vector2 GetFrameSize(Texture2D texture, float frameCount, float scale);
```

```cpp
// Sprite.cpp
#include "Sprite.h"

Vector2 GetFrameSize(Texture2D texture, float frameCount, float scale) {
    return {
        (float)texture.width / frameCount * scale,
        (float)texture.height * scale,
    };
}
```

The Makefile's `wildcard *.cpp` picks it up automatically. Now define the scale
in exactly one place and derive every size:

```cpp
const float SPRITE_SCALE = 2.0f; // one knob, one meaning
const float FRAME_COUNT  = 4.0f; // every sheet in this pack has 4 frames

player.size = GetFrameSize(playerTexture, FRAME_COUNT, SPRITE_SCALE); // 40x46
e.size      = GetFrameSize(enemyTexture,  FRAME_COUNT, SPRITE_SCALE); // 40x46
b.size      = GetFrameSize(bulletTexture, 1.0f,        SPRITE_SCALE); // 6x12
```

(Change `Player.size` from `int` to `Vector2` in `Player.h` — a ship is 2D; an
`int` can only hold one dimension, which is exactly why the player was secretly
40 tall but 46 drawn.)

### Why this wins

Nothing in `DrawEnemies`, `UpdateEnemies`, `DrawBullets`, or the movement clamps
re-typed a number — they all *read* `e.size` / `b.size` / `player.size`:

- collision box = `e.size` = **derived from the texture**
- drawn rectangle = `e.size` = **the same field**

Same rectangle, by construction. The invisible box physically cannot drift
from the art anymore. The scale knob lives in one line, not scattered as `2.0f`
inside draw functions.

Two small cleanups while you're here (both are the same disease):

1. `DrawPlayer`'s `dest` should read `player.size`, not recompute
   `frameWidth * 2.0f`.
2. The bullet spawn offset `- 3` assumed a 6-wide bullet. Make it a formula:
   `player.position.x + player.size.x / 2.0f - b.size.x / 2.0f` — ship center
   minus bullet half-width. Now it's correct for *any* sizes.

---

## Part 6 — Prove it

Build and run: same game, same numbers — but now computed, not typed.

Now change **only a path**:

```cpp
Texture2D playerTexture = LoadTexture("assets/spaceships/spr_spaceship_02_animation.png");
```

That file is **88×24** — a different size. Change nothing else. The ship draws
at its own size, clamps correctly, bullets leave its exact center, and (once
you add player-enemy collision next chapter) the hitbox is right the first
time. *That* is what "sizes follow the texture" feels like: change art, not
code.

---

## Common Mistakes

| Mistake | Why it hurts |
|---|---|
| Hardcoded `{20, 23}` sizes | duplicates the texture's knowledge; drifts silently into ghost hitboxes |
| `int size` on the player | a 2D thing held in a 1D number → drawn ≠ collided |
| Wrong `frameCount` | collision box becomes a fraction of the sprite. Use the named constant |
| Loading inside the loop / before `InitWindow` | slow, leaks VRAM, or crashes |
| `* 2.0f` inside draw functions | the scale drifts from `SPRITE_SCALE`; it must live in one place |
| `source` beyond the image | blank/corner sprites |

---

## Homework

**1 — Everything above.** Real sprites, flickering flame, enemies hittable at
the wing tips, sizes derived.

**2 — 1× mode.** Set `SPRITE_SCALE = 1.0f`. Ships shrink, and their collision
and clamps shrink with them. Put it back.

**3 — Swap art.** Switch the player path to `spr_spaceship_07_animation.png`
(144×24). Zero code changes. Then the enemy to `spr_enemy_spaceship_02_animation.png`
(120×25).

**4 — Animate enemies separately.** Give each `Enemy` its own `animOffset`
(`(float)GetRandomValue(0, 100)`), and draw frame
`(int)((GetTime() + e.animOffset) / 0.1f) % 4`. The robotic synchronized flicker
becomes alive.

---

## Next

**Chapter 07 — SOLID & Abstract Entity Classes.** You now have real objects
whose sizes come from the texture. The next chapter answers "I can't build an
abstract class in my own game" — a base `Entity` behind Player/Enemy/Bullet, and
the five SOLID principles tied to that one refactor. After that: sound.