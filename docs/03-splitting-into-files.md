# Chapter 03 — Splitting Into Multiple Files

## What You Will Learn

- Why putting everything in one file is a problem
- What a header file (`.h`) is and what it does
- What a source file (`.cpp`) is and what it does
- The `#pragma once` guard and why you need it
- How to `#include` your own files
- How to split `Player` out of `main.cpp` cleanly

---

## Why Split Into Files?

Right now your entire game fits in one file: `main.cpp`.
That is fine while the game is small.

But think about what is coming:
- Enemies
- Bullets
- Explosions
- Score system
- Sound
- Menu screen

If all of that lives in `main.cpp`, you will end up with 1,000+
lines in a single file. Finding things becomes painful.
Changing one thing breaks another.

The solution: **give each "thing" its own pair of files**.

```
main.cpp       ← the game director
Player.h       ← declares what Player IS
Player.cpp     ← defines how Player WORKS
```

This is the standard pattern in every real C++ project.
You will use it for the rest of this game.

---

## The Two File Types

### Header file — `.h`

A header file is a **declaration**.
It answers the question: **"what exists?"**

It tells the compiler:
- "There is a struct called `Player` that looks like this"
- "There is a function called `UpdatePlayer` that takes these arguments"

It does **not** contain the actual code that does the work.
Just the shape and names.

```cpp
// Player.h — declarations only
struct Player {
    Vector2 position;
    float speed;
    int size;
};

void DrawPlayer(Player player);
void UpdatePlayer(Player& player);
```

### Source file — `.cpp`

A source file is a **definition**.
It answers the question: **"how does it actually work?"**

This is where the real code lives.

```cpp
// Player.cpp — the actual implementations
#include "Player.h"

void DrawPlayer(Player player)
{
    // ... real code here
}

void UpdatePlayer(Player& player)
{
    // ... real code here
}
```

---

## The Include Guard — `#pragma once`

When you split into files, multiple `.cpp` files might include
the same header. Without protection, the compiler would see the
same struct declared twice and throw an error.

The fix is one line at the very top of every header file:

```cpp
#pragma once
```

This tells the compiler: **"include this file only once,
no matter how many times someone tries to include it"**.

Always put it at the top of every `.h` file. No exceptions.

---

## How `#include` Works With Your Own Files

You already know:

```cpp
#include "raylib.h"   // includes the raylib library
```

The same syntax works for your own files.
The difference is **angle brackets vs quotes**:

| Syntax | Used for |
|---|---|
| `#include <raylib.h>` | System / installed libraries |
| `#include "Player.h"` | Your own files in the same folder |

Use quotes for everything you write yourself.

---

## The Build: Why `Player.cpp` Must Be Compiled Too

When you split code into `Player.cpp`, the compiler needs to
know that file exists. Your build command changes from:

```bash
# Before — only one file
clang++ main.cpp -o space-shooter -lraylib ...

# After — both files
clang++ main.cpp Player.cpp -o space-shooter -lraylib ...
```

If you forget `Player.cpp`, the linker will scream at you:
```
undefined reference to `UpdatePlayer`
```

That error means: "I know this function was declared (from the header),
but I cannot find the actual code for it."
The fix is always: add the missing `.cpp` to your build command.

---

## Step by Step — Do This Now

### Step 1 — Create `Player.h`

Create a new file called `Player.h` in the same folder as `main.cpp`.

```cpp
#pragma once

#include "raylib.h"

struct Player {
    Vector2 position;
    float speed;
    int size;
};

void DrawPlayer(Player player);
void UpdatePlayer(Player& player);
```

That is the entire file. Just the declarations.
No function bodies. No `main()`.

---

### Step 2 — Create `Player.cpp`

Create a new file called `Player.cpp` in the same folder.

```cpp
#include "Player.h"

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

void UpdatePlayer(Player& player)
{
    float delta = GetFrameTime();

    if (IsKeyDown(KEY_W)) player.position.y -= player.speed * delta;
    if (IsKeyDown(KEY_S)) player.position.y += player.speed * delta;
    if (IsKeyDown(KEY_A)) player.position.x -= player.speed * delta;
    if (IsKeyDown(KEY_D)) player.position.x += player.speed * delta;
}
```

Notice: `Player.cpp` includes `Player.h`, not `raylib.h` directly.
`Player.h` already includes `raylib.h`, so you get it automatically.
No need to include it twice.

---

### Step 3 — Clean Up `main.cpp`

Now that `Player` lives in its own files, rip it out of `main.cpp`.

Your `main.cpp` becomes much cleaner:

```cpp
#include "raylib.h"
#include "Player.h"

int main()
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Pesawat Tempur");
    SetTargetFPS(90);

    Player player;
    player.position = { 400, 200 };
    player.size = 40;
    player.speed = 200.0f;

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

`main.cpp` no longer contains any struct or function definitions.
It only includes what it needs and runs the game.
That is its only job.

---

### Step 4 — Update Your Build Command

Open `.vscode/tasks.json`. Find the compile command and add `Player.cpp`.

It should look something like this (exact flags may vary):

```json
"command": "clang++",
"args": [
    "main.cpp",
    "Player.cpp",
    "-o",
    "space-shooter",
    "-lraylib",
    ...
]
```

Add `"Player.cpp"` right after `"main.cpp"`.

---

### Step 5 — Build and Run

Build the project. If everything is correct, it compiles with no errors
and the game behaves exactly the same as before.

**Nothing visible changed for the player. Everything changed for you.**

That is the point of a refactor: same behavior, better structure.

---

## What Your Project Looks Like Now

```
space-shooter/
├── main.cpp        ← game director, the loop
├── Player.h        ← what a Player IS (struct + function signatures)
├── Player.cpp      ← how Player WORKS (function bodies)
└── ...
```

This is the pattern for every new "thing" you add:

| You add... | You create... |
|---|---|
| Enemies | `Enemy.h` + `Enemy.cpp` |
| Bullets | `Bullet.h` + `Bullet.cpp` |
| Score display | `HUD.h` + `HUD.cpp` |

`main.cpp` just grows a few new `#include` lines.
Each feature stays in its own tidy box.

---

## Common Mistakes

### Mistake 1 — Forgetting `#pragma once`
Without it, if two files include the same header you get:
```
error: redefinition of 'struct Player'
```
Fix: add `#pragma once` at the top of every `.h` file.

### Mistake 2 — Forgetting to compile `Player.cpp`
The compiler does not automatically find `.cpp` files.
You must list every `.cpp` in the build command.
Symptom: `undefined reference to UpdatePlayer`.
Fix: add `Player.cpp` to the build command.

### Mistake 3 — Putting function bodies in the header
You can, but you should not yet. It causes problems when multiple
`.cpp` files include the same header — the function gets defined
more than once and the linker complains.
Rule for now: **declarations go in `.h`, definitions go in `.cpp`**.

### Mistake 4 — Including `Player.cpp` instead of `Player.h`
Always include the header, not the source file.
`#include "Player.h"` ✅
`#include "Player.cpp"` ❌

---

## Why Does This Matter?

Right now splitting three functions across two files feels like
extra work for no reason. The game is exactly the same.

But next chapter you will add **screen clamping** — making sure
the player cannot walk off screen. Where does that code go?
`UpdatePlayer()`, inside `Player.cpp`. You do not touch `main.cpp`.

After that you will add **enemies**. Where does that code go?
`Enemy.h` and `Enemy.cpp`. You do not touch `Player.cpp`.

Each feature stays in its lane. That is why this structure exists.

---

## Homework

Do these in order. Build and run after each one.

**Homework 1 — Split the files**
Create `Player.h`, `Player.cpp`, and clean up `main.cpp`
exactly as described above. The game must run identically.

**Homework 2 — Add screen clamping in `Player.cpp`**
Inside `UpdatePlayer()`, after the movement code, add boundary checks.
The player must not be able to go off screen.

```cpp
// Clamp X
if (player.position.x < 0) player.position.x = 0;
if (player.position.x > 800 - player.size) player.position.x = 800 - player.size;

// Clamp Y
if (player.position.y < 0) player.position.y = 0;
if (player.position.y > 450 - player.size) player.position.y = 450 - player.size;
```

Notice: `800` and `450` are hardcoded here, which is not ideal.
Can you think of a cleaner way? (Hint: look at how `main()` stores them.)
You do not need to fix it now — just notice the problem.

**Homework 3 — Add a speed display**
In `main.cpp`, inside the draw block, add this line:

```cpp
DrawText(TextFormat("Speed: %.0f", player.speed), 10, 10, 20, DARKGRAY);
```

`TextFormat` works like printf — `%.0f` means "a float with 0 decimal places".
This will display the current speed in the top-left corner.

**Homework 4 (challenge) — Move the screen size to `main.cpp` constants**
Right now `UpdatePlayer()` uses the raw numbers `800` and `450` for clamping.
That is fragile — if you ever change the screen size, you must remember to
update `Player.cpp` too.

A cleaner solution: pass the screen size as arguments.

Change the signature to:
```cpp
void UpdatePlayer(Player& player, int screenWidth, int screenHeight);
```

Update `Player.h`, `Player.cpp`, and the call in `main.cpp` accordingly.

---

## Next

→ Chapter 04 — Shooting Bullets (vectors, spawning, and cleanup)
