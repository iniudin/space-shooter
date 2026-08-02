# Chapter 04 — Shooting Bullets

## What You Will Learn

- What `std::vector` is and why you need it for bullets
- How to create a `Bullet` struct and give it its own files
- How to spawn a bullet when the player presses Space
- How to move all bullets every frame
- How to delete bullets that go off screen
- Why cleanup matters and what happens if you skip it

---

## The Problem With One Bullet

Imagine you try to make a bullet using what you already know:

```cpp
Bullet bullet;
bullet.position = player.position;
bullet.active = true;
```

This works for exactly one bullet at a time.
The moment the player fires again, you overwrite the first bullet.

Real games need many bullets alive at the same time.
The player fires at frame 10. Fires again at frame 25.
Fires again at frame 30. All three should exist simultaneously,
moving up the screen until they leave or hit something.

One variable cannot do that. You need a **list** of bullets.

---

## What is `std::vector`?

`std::vector` is a **resizable list** from the C++ standard library.
It holds multiple values of the same type, and you can add or
remove items from it while the game is running.

```cpp
std::vector<Bullet> bullets;  // an empty list of bullets
```

You can think of it like a row of boxes that grows when you
need more space and shrinks when you remove items.

### Adding to the list

```cpp
Bullet b;
b.position = { 400, 200 };
bullets.push_back(b);  // add b to the end of the list
```

`push_back` means "put this at the back of the list".

### Looping through the list

```cpp
for (Bullet& b : bullets) {
    b.position.y -= 400 * delta;  // move each bullet upward
}
```

This is a **range-based for loop**.
It reads: "for every bullet `b` in the list `bullets`, do this".
The `&` means you are working on the real bullet, not a copy.
Without `&`, changes would be lost — same reason as `UpdatePlayer`.

### Removing items

```cpp
bullets.erase(bullets.begin() + i);
```

This is how you remove one item at a specific position `i` from the list.
You will see this in detail during the cleanup step.

### The include you need

`std::vector` lives in a standard library header.
Add this at the top of any file that uses it:

```cpp
#include <vector>
```

Note the angle brackets — this is a system library, not your own file.

---

## The Bullet Struct

A bullet needs very little data:

```cpp
struct Bullet {
    Vector2 position;
    float speed;
};
```

That is it. Position and speed.
No size field — a bullet will always be the same small rectangle.
No color field — bullets will always be the same color.
Keep structs as small as they need to be. Add fields only when you need them.

---

## The Flow of a Bullet's Life

Before writing any code, understand what a bullet does every frame:

```
SPAWN   → player presses Space → a new Bullet appears at player's position
UPDATE  → every frame, each bullet moves upward
CLEANUP → if a bullet's y position goes above 0 (off screen top), remove it
DRAW    → draw every remaining bullet
```

This is the same UPDATE → DRAW pattern from the game loop,
applied to a list of objects instead of one.

---

## Step by Step — Do This Now

### Step 1 — Create `Bullet.h`

Create a new file `Bullet.h`.

```cpp
#pragma once

#include "raylib.h"

struct Bullet {
    Vector2 position;
    float speed;
};

void UpdateBullets(std::vector<Bullet>& bullets);
void DrawBullets(const std::vector<Bullet>& bullets);
```

Wait — `std::vector` is used here, so `Bullet.h` needs to know what it is.
Add the include at the top:

```cpp
#pragma once

#include <vector>
#include "raylib.h"

struct Bullet {
    Vector2 position;
    float speed;
};

void UpdateBullets(std::vector<Bullet>& bullets);
void DrawBullets(const std::vector<Bullet>& bullets);
```

Notice two things about the function signatures:

**`UpdateBullets` takes `std::vector<Bullet>&`** — with `&`.
The function needs to modify the list (move bullets, remove them).
Without `&`, it gets a copy and all changes are lost.

**`DrawBullets` takes `const std::vector<Bullet>&`** — with `const` and `&`.
Drawing does not change anything. `const` makes that explicit —
if you accidentally write to a bullet inside `DrawBullets`, the compiler
will catch it and tell you. `&` avoids copying the entire list just to read it.

---

### Step 2 — Create `Bullet.cpp`

Create a new file `Bullet.cpp`.

```cpp
#include "Bullet.h"

void UpdateBullets(std::vector<Bullet>& bullets) {
    float delta = GetFrameTime();

    // Move all bullets upward
    for (Bullet& b : bullets) {
        b.position.y -= b.speed * delta;
    }

    // Remove bullets that have gone off the top of the screen
    for (int i = (int)bullets.size() - 1; i >= 0; i--) {
        if (bullets[i].position.y < 0) {
            bullets.erase(bullets.begin() + i);
        }
    }
}

void DrawBullets(const std::vector<Bullet>& bullets) {
    for (const Bullet& b : bullets) {
        DrawRectangle(
            (int)b.position.x,
            (int)b.position.y,
            6,   // width
            16,  // height
            YELLOW
        );
    }
}
```

There is something unusual about the cleanup loop. Let's look at it carefully.

---

## Why the Cleanup Loop Goes Backward

The movement loop goes **forward** — from the first bullet to the last.
The cleanup loop goes **backward** — from the last bullet to the first.

Why?

Imagine you have 3 bullets in the list at positions `[0]`, `[1]`, `[2]`.
You are looping forward: `i = 0, 1, 2`.

You check `[0]` — it is off screen. You erase it.
The list shifts: what was `[1]` is now `[0]`. What was `[2]` is now `[1]`.

Now you move to `i = 1`. But `[1]` is now what used to be `[2]`.
You skipped `[0]` (the bullet that just shifted down). It never gets checked.

```
Before erase:  [A] [B] [C]
               i=0
Erase A:       [B] [C]
               i=1  ← skips B entirely!
```

Going **backward** avoids this problem entirely.
When you erase item `[i]`, everything after it shifts left.
But you are moving left anyway — toward smaller indices.
Items you have not checked yet are to the left and are unaffected.

```
Before:   [A] [B] [C]
               i=2
Check C:  it's off screen, erase it → [A] [B]
               i=1
Check B:  fine, keep it
               i=0
Check A:  off screen, erase it → [B]
Done.
```

Every bullet gets checked. Nothing is skipped.

This is a pattern you will use for the rest of your career
whenever you delete items from a list while iterating over it.
Memorize it.

---

### Step 3 — Add Spawning Logic

Spawning a bullet means creating a new `Bullet` and adding it to the list.
This happens when the player presses Space.

Where does this code go?

Not in `Bullet.cpp` — a bullet should not know anything about the player.
Not in `Player.cpp` — the player should not know anything about bullets.
It goes in `main.cpp`, in the update section. `main()` is the director.
It knows about both the player and the bullet list, so it connects them.

```cpp
// In main.cpp, inside the game loop, in the UPDATE section:
if (IsKeyPressed(KEY_SPACE)) {
    Bullet b;
    b.position = player.position;
    b.speed = 500.0f;
    bullets.push_back(b);
}
```

Notice `IsKeyPressed` here, not `IsKeyDown`.

`IsKeyDown(KEY_SPACE)` would fire a new bullet every single frame
while Space is held. At 90 FPS that is 90 bullets per second.
`IsKeyPressed(KEY_SPACE)` fires only on the first frame of the press.
One press = one bullet. That is what you want.

Also notice the bullet spawns at `player.position`.
This puts it at the top-left corner of the player box.
Later you might want to center it:

```cpp
b.position.x = player.position.x + player.size / 2.0f - 3; // centered
b.position.y = player.position.y;
```

But `player.position` is fine for now. Get it working first.

---

### Step 4 — Update `main.cpp`

Your full `main.cpp` should now look like this:

```cpp
#include "raylib.h"
#include "Player.h"
#include "Bullet.h"
#include "HUD.h"
#include <vector>

int main() {
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Pesawat Tempur");
    SetTargetFPS(90);

    Player player;
    player.position = { 400, 200 };
    player.size = 40;
    player.speed = 200.0f;

    std::vector<Bullet> bullets;

    while (!WindowShouldClose()) {
        // --- UPDATE ---
        UpdatePlayer(player, screenWidth, screenHeight);
        UpdateBullets(bullets);

        if (IsKeyPressed(KEY_SPACE)) {
            Bullet b;
            b.position = player.position;
            b.speed = 500.0f;
            bullets.push_back(b);
        }

        // --- DRAW ---
        BeginDrawing();
        ClearBackground(DARKGRAY);
        DrawPlayer(player);
        DrawBullets(bullets);
        DrawSpeed(player);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
```

`main()` creates the list. `main()` handles spawning.
`UpdateBullets` handles movement and cleanup.
`DrawBullets` handles drawing.
Each piece knows only what it needs to know.

---

### Step 5 — Update the Makefile

`Bullet.cpp` is a new source file. The build system needs to know it exists.

Open your `Makefile`. Find where `Player.cpp` and `HUD.cpp` are listed.
You should see something like:

```makefile
SRCS = main.cpp Player.cpp HUD.cpp
```

Add `Bullet.cpp`:

```makefile
SRCS = main.cpp Player.cpp HUD.cpp Bullet.cpp
```

If you forget this step, the linker will complain:

```
Undefined symbols: "UpdateBullets", "DrawBullets"
```

That error means: "I saw the declaration in `Bullet.h`, but I cannot find
the actual code." The fix is always: add the `.cpp` to the build.

---

### Step 6 — Build and Run

Build the project. Fix any errors. Then test:

- Press Space — a bullet should appear and move upward
- Hold Space — only one bullet fires per press
- Move while firing — bullets should spawn at the player's position
- Keep firing — old bullets disappear once they leave the screen

If bullets are not appearing, check that `DrawBullets` is called
between `BeginDrawing()` and `EndDrawing()`.

If bullets appear but do not move, check that `UpdateBullets` is called
before `BeginDrawing()`, in the update section.

---

## What Your Project Looks Like Now

```
space-shooter/
├── main.cpp        ← director: loop, spawning, wiring
├── Player.h/.cpp   ← what a Player is and does
├── Bullet.h/.cpp   ← what a Bullet is and does
├── HUD.h/.cpp      ← what the HUD displays
└── Makefile
```

Every new "thing" in the game gets its own pair of files.
`main.cpp` grows one `#include` and a few lines. That is all.

---

## Common Mistakes

### Mistake 1 — Using `IsKeyDown` instead of `IsKeyPressed` for spawning
`IsKeyDown` fires every frame the key is held. At 90 FPS you get a wall of
bullets instantly. Use `IsKeyPressed` for one-shot actions like firing.

### Mistake 2 — Forgetting `&` in the loop
```cpp
for (Bullet b : bullets) {     // b is a copy — changes are lost
for (Bullet& b : bullets) {    // b is the real thing — changes stick
```
Always use `&` when you want to modify items in the loop.

### Mistake 3 — Erasing while looping forward
Covered in depth above. Always iterate backward when erasing from a vector.

### Mistake 4 — Forgetting `Bullet.cpp` in the build
The Makefile must list every `.cpp` file. Missing one = linker error.

### Mistake 5 — Drawing outside BeginDrawing/EndDrawing
If `DrawBullets` is called outside that block, nothing will appear.
All draw calls must be between `BeginDrawing()` and `EndDrawing()`.

---

## Homework

Do these in order. Build and run after each one.

**Homework 1 — Get it working**
Implement everything above. Bullets should fire, move, and disappear.

**Homework 2 — Center the bullet on the player**
Right now bullets spawn at `player.position`, which is the top-left corner
of the player box. The bullet looks like it shoots from the left edge.

Fix it so the bullet spawns at the horizontal center of the player:

```cpp
b.position.x = player.position.x + player.size / 2.0f - 3;
b.position.y = player.position.y;
```

The `- 3` accounts for half the bullet's width (6 pixels wide).
Why does this make it look more centered?

**Homework 3 — Add a bullet counter to the HUD**
In `HUD.h`, add a new function:

```cpp
void DrawBulletCount(int count);
```

In `HUD.cpp`, implement it:

```cpp
void DrawBulletCount(int count) {
    DrawText(TextFormat("Bullets: %d", count), 10, 50, 20, YELLOW);
}
```

Call it in `main.cpp`:

```cpp
DrawBulletCount((int)bullets.size());
```

`%d` is the format specifier for integers (whole numbers).
`bullets.size()` returns the current number of bullets in the list.
The `(int)` cast is needed because `size()` returns an unsigned type.

Watch the number go up as you fire and down as bullets leave the screen.

**Homework 4 — Add a fire rate limit (challenge)**
Right now you can only fire once per key press. But what if you hold Space?
Nothing happens because `IsKeyPressed` only triggers once.

Change the game so holding Space fires continuously,
but at a controlled rate — one bullet every 0.15 seconds.

You will need:
- A `float fireTimer = 0.0f;` variable in `main()`
- Add `GetFrameTime()` to it each frame
- When it reaches 0.15, spawn a bullet and reset it to 0

```cpp
fireTimer += GetFrameTime();

if (IsKeyDown(KEY_SPACE) && fireTimer >= 0.15f) {
    Bullet b;
    b.position = player.position;
    b.speed = 500.0f;
    bullets.push_back(b);
    fireTimer = 0.0f;
}
```

Try changing `0.15f` to `0.05f` (faster) and `0.5f` (slower).
What feels best for your game?

---

## Next

→ Chapter 05 — Enemies (spawning, movement patterns, and collision detection)
