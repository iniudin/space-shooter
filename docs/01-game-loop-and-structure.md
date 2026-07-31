# Chapter 01 — Game Loop & Project Structure

## What You Will Learn

- What a game loop is and why it exists
- The difference between UPDATE and DRAW
- Why keeping your code modular saves your sanity
- How your current `main.cpp` works line by line

---

## Why Does a Game Need a Loop?

Think about any game you have ever played.
The character moves. Enemies chase you. The score goes up.
Everything is constantly changing — every single second.

How does the computer do that?

It runs the same block of code **over and over again**, very fast.
That block of code is called the **game loop**.

In your code, this is the game loop:

```cpp
while (!WindowShouldClose())
{
    // this runs ~90 times per second
}
```

`while` means "keep doing this as long as the condition is true".
`WindowShouldClose()` is a raylib function that returns `true`
when the player closes the window or presses Escape.
The `!` means "not" — so the whole thing reads:
**"keep looping as long as the window is NOT closed"**.

Every time the loop runs once, that is called a **frame**.
At 90 FPS (frames per second), the loop runs 90 times every second.

---

## What Happens Inside the Loop?

Every frame, a game does exactly two things — in this order:

```
1. UPDATE  → think: move things, read input, run game logic
2. DRAW    → think: paint everything onto the screen
```

This order matters. You must finish all your thinking (update)
before you start painting (draw). If you mix them, things break
in confusing ways.

Here is what your loop should look like conceptually:

```cpp
while (!WindowShouldClose())
{
    // --- UPDATE ---
    // read keyboard, move player, check collisions, etc.

    // --- DRAW ---
    BeginDrawing();
        ClearBackground(RAYWHITE);
        // draw everything here
    EndDrawing();
}
```

### What does BeginDrawing() and EndDrawing() do?

Raylib uses a technique called **double buffering**.
Imagine two whiteboards. While the player is looking at one,
you are drawing on the other. When you are done drawing,
you flip them. This makes the screen look smooth with no flicker.

- `BeginDrawing()` — tells raylib "I am starting to draw on the back whiteboard"
- `EndDrawing()` — tells raylib "I am done, flip the whiteboards"

Everything you draw must be between these two calls.

### What does ClearBackground() do?

Before drawing anything new, you need to erase the previous frame.
`ClearBackground(RAYWHITE)` paints the entire screen one solid color.
`RAYWHITE` is a raylib color constant (it is a very light white).

If you skip this, the previous frame stays on screen and new frames
get drawn on top of it — like drawing on dirty paper.

---

## Your Current Code, Line by Line

```cpp
#include "raylib.h"
```
This line imports the raylib library.
`#include` means "paste the contents of this file here".
Without it, the compiler does not know what `InitWindow` or
`DrawText` means.

```cpp
int main()
```
Every C++ program must have a `main` function.
This is where the program starts running.
`int` means the function will return a number when it finishes.
By convention, returning `0` means "everything went fine".

```cpp
const int screenWidth = 800;
const int screenHeight = 450;
```
`const` means this value will never change.
`int` means it is a whole number (no decimals).
These are just variables to store the window size so you do not
have to type `800` and `450` everywhere.

```cpp
InitWindow(screenWidth, screenHeight, "Pesawat Tempur");
```
This creates the game window.
First argument: width in pixels.
Second argument: height in pixels.
Third argument: the title shown in the title bar.

```cpp
SetTargetFPS(90);
```
This tells raylib to try to run the loop 90 times per second.
Without this, the loop runs as fast as the CPU allows,
which could be thousands of times per second — wasteful.

```cpp
CloseWindow();
```
This cleans up and releases memory when the game exits.
Always call this at the end.

```cpp
return 0;
```
Tells the operating system the program finished without errors.

---

## Why Modular Code?

Right now your entire game is inside `main()`.
That is fine when the game is tiny.

But imagine in the future you have:
- A player
- 10 types of enemies
- Bullets
- A score system
- Sound effects
- A menu screen

If all of that is inside `main()`, it will be hundreds of lines
tangled together. Changing one thing breaks another.
That is spaghetti code.

The solution is simple: **each "thing" in your game gets its own
chunk of code**. A struct for its data. A function to update it.
A function to draw it. `main()` just connects them.

```
main.cpp
├── struct Player       ← data: what a player IS (position, speed, size)
├── UpdatePlayer()      ← behavior: what a player DOES (move, react to input)
├── DrawPlayer()        ← appearance: what a player LOOKS LIKE (a box)
└── main()              ← director: creates things and runs the loop
```

This pattern scales to any game size. You will use it forever.

---

## Homework

**Goal:** understand the loop, not write code yet.

1. Change `SetTargetFPS(90)` to `SetTargetFPS(5)`.
   Build and run. What do you notice? Why does it look like that?
   Change it back to 90 when done.

2. Change `ClearBackground(RAYWHITE)` to `ClearBackground(BLACK)`.
   Build and run. What changed?
   Now try removing `ClearBackground(...)` entirely.
   What happens and why?

3. Change the window title from `"Pesawat Tempur"` to your own title.

4. In your own words, write down the answer to:
   "What is the difference between UPDATE and DRAW?
   Why must UPDATE happen before DRAW?"

You do not need to look anything up. The answer is in this chapter.

---

## Next

→ [Chapter 02 — Player Box & Keyboard Movement](02-player-box-and-movement.md)
