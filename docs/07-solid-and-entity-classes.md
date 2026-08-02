# Chapter 07 — SOLID & Abstract Entity Classes

This is the chapter that answers the feeling you have had since Chapter 06:

> *"I can't build an abstract class in my own game."*

Good news: that feeling is **correct** — but not for the reason you think. It is
not a gap in your skill. It is that until now your code gave you no good *place*
for an abstraction to live. This chapter names that place and hands you the
toolbox — the five SOLID principles — to put one there cleanly.

It is also a warning: **do not sprinkle abstract classes everywhere because they
sound fancy.** Abstraction has a real cost (indirection, boilerplate, slightly
harder debugging). The best place to *feel* that cost is a tiny game like this —
where you can weigh it against the payoff on a single screen.

---

## What You Will Learn

- Why abstraction "won't fit" your code yet (the god function, structs, free
  functions)
- What SOLID stands for, each letter tied to a real change in *this* game
- How to write an abstract base class `Entity` — a class you cannot instantiate,
  only subclass
- `virtual`, pure virtual (`= 0`), `override`, and the virtual destructor
- How the game loop gets *shorter* when entities Update and Draw themselves
- `std::vector<std::unique_ptr<Entity>>` — a polymorphic container
- Why collision is the one place abstraction does *not* cleanly fit, and what to
  do about it (a `kind()` tag)
- The three biggest traps: "abstraction theatre", a fat base class, and the
  `dynamic_cast` habit

---

## Part 1 — Why abstraction won't fit your code today

Open `main.cpp`. Count how many jobs that one file does:

1. creates the window
2. loads and checks all four textures
3. runs the enemy spawner (and the buggy `gameTime` we fixed in ch07)
4. fires bullets
5. updates the player, enemies, bullets, and the animation frame
6. draws every `DrawXxx`, plus the HUD

That is a **God Function**: too many responsibilities squeezed into one place.
An abstract class needs a boundary to *live behind*, but a god function has no
walls — so there is nowhere to draw it.

Meanwhile your entities are plain `struct` data bags, and their behavior lives
in free functions named after the type: `UpdateEnemies`, `UpdateBullets`,
`DrawEnemies`, `DrawPlayer`. The code knows each type by **name**, not by a
**common interface**. So there is nothing yet to inherit *from*.

Then, the moment you start typing `class Enemy`, you hit a wall: "which methods
do I actually share? Update? Draw? But Enemy and Bullet need different Draw
bodies..." — and you give up. That is the *exact* moment you feel *"I can't
make an abstract class."*

The reframe:

> **Abstraction is not a trophy to collect. It is a boundary you draw where a
> family of things behaves the same.**

You have exactly one such family here: **"every object on the field that updates
once per frame and draws once per frame."** Player, Enemy, Bullet — and
tomorrow PowerUp, Asteroid — are all members. That family is your abstract
class.

---

## Part 2 — The five principles on one ruler

Keep this table. Every decision in the rest of the chapter is one of these:

| Letter | Name | In this game |
|---|---|---|
| **S** | Single Responsibility | each class does one job; Game no longer stores + moves every bullet by hand |
| **O** | Open for extension, **Closed** for modification | add a new entity kind by writing a class; do NOT edit the loop to add it |
| **L** | Liskov Substitution | a subclass behaves like an `Entity`, so one loop over `unique_ptr<Entity>` is safe for all of them |
| **I** | Interface Segregation | keep the base thin; don't force `Bullet` (or a Power-Up) to fake fields it ignores |
| **D** | Dependency Inversion | the Game loop depends on abstract `Entity`, not on `Player` / `Enemy` / `Bullet` by name |

Read them as a chain: **S** gives each class one job, **O** makes new kinds
cheap to add, **L** makes "treat everyone as Entity" legal, **I** stops you
from polluting the base with irrelevant junk, **D** turns "I know every type by
hand" into "I handle the family."

---

## Part 3 — The abstract base: `Entity`

Create a new file pair. This handful of lines is the heart of the chapter.

**Entity.h**

```cpp
#pragma once
#include "raylib.h"

// A "tag" used later to route collisions. We explain it in Part 7.
enum class EntityKind {
    Player,
    Enemy,
    Bullet,
    // add PowerUp, Asteroid ... as you add new kinds
};

// Abstract base class: "an object on the field that updates and draws."
class Entity {
public:
    Vector2 position{};
    Vector2 size{};
    bool active = true;

    Entity() = default;
    virtual ~Entity() = default;          // REQUIRED with polymorphic pointers

    virtual void Update(float dt) = 0;    // pure virtual -> Entity is abstract
    virtual void Draw() const = 0;

    virtual Rectangle bounds() const;     // shared default: position + size
    virtual EntityKind kind() const = 0;  // collision tag, see Part 7
};

inline Rectangle Entity::bounds() const {
    return { position.x, position.y, size.x, size.y };
}
```

Three details that matter:

- **`= 0` is a pure virtual.** A class with even one of those *cannot* be
  created directly (`Entity e;` is a compile error). It exists only as a base.
  That is the C++ spelling of **abstract class**.
- **`virtual ~Entity()`.** If you ever hold an `Entity*` and delete through it,
  the compiler must pick the *derived* destructor. Without this line,
  `std::unique_ptr<Entity>` deleting a `Bullet` is undefined behavior. Rule:
  any polymorphic class gets a virtual destructor.
- **`bounds()`** is the rectangle math you already wrote in `UpdateEnemies`:
  `{ position.x, position.y, size.x, size.y }`. And it honors Chapter 06:
  `size` comes from the texture via `GetFrameSize`.

---

## Part 4 — The concrete children

**Enemy.h**

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
    void TakeDamage(int amount) {
        health -= amount;
        active = health > 0;             // dies when health hits 0
    }

private:
    Texture2D texture;
    int health;
    float speed;
    float animTimer;
};
```

**Enemy.cpp**

```cpp
#include "Enemy.h"
#include "Sprite.h"                     // GetFrameSize from Chapter 06

Enemy::Enemy(Texture2D texture, Vector2 start, int health, float speed)
    : texture(texture), health(health), speed(speed), animTimer(0.0f) {
    position = start;
    size = GetFrameSize(texture, 4.0f, 2.0f);   // 4 frames, 2x scale
}

void Enemy::Update(float dt) {
    position.y += speed * dt;                   // fall down
    animTimer += dt;
    if (position.y > 480.0f) active = false;    // fell off the bottom
}

void Enemy::Draw() const {
    int frame = (int)(animTimer / 0.1f) % 4;
    float frameWidth = (float)texture.width / 4.0f;
    Rectangle source = { frame * frameWidth, 0.0f,
                         frameWidth, (float)texture.height };
    Rectangle dest = bounds();                  // the SAME rectangle as collision
    DrawTexturePro(texture, source, dest, {0, 0}, 0.0f, WHITE);
}
```

Notice: `Update` and `Draw` only describe **this one enemy's rules**. The loop
that runs them will not know which class it is. That is the whole point.

Mirror this pattern for the other two children — the shape is identical, only
the behavior differs.

**Player.h / Player.cpp** — arrow keys, clamps to the window edges, stores
`screenW`, `screenH`, `speed`, plus `animTimer` for the flame.

**Bullet.h / Bullet.cpp** — a `damage` value (say `1`), moves upward, turns
`active = false` off the top. Its `size` also comes from `GetFrameSize`.

The thing to *feel*: before, `UpdateEnemies`, `UpdateBullets`, `DrawPlayer`, ...
were separate functions you had to call one by one. Now every concrete class
owns its behavior, and the game loop just says "update all" and "draw all".

---

## Part 5 — The container: polymorphism in practice

Because all three are `Entity`, a Game can hold them in one container:

```cpp
std::vector<std::unique_ptr<Entity>> things;  // needs #include <memory>
```

Why `std::unique_ptr<Entity>` instead of raw `Entity*`? **Ownership.** When a
`unique_ptr` leaves scope it `delete`s its object *through the base pointer* —
which is why the virtual destructor existed above. No manual delete, no leaks.

Adding objects:

```cpp
things.push_back(std::make_unique<Bullet>(bulletTexture, pos, 500.0f));
things.push_back(std::make_unique<Enemy>(enemyTexture, start, health, speed));
```

Running the whole field:

```cpp
// UPDATE
for (auto &thing : things) if (thing->active) thing->Update(dt);

// DRAW
for (auto &thing : things) if (thing->active) thing->Draw();
```

That tiny double loop is where "abstract class" stops being a buzzword.
**Liskov (L)** makes it legal: every subclass behaves enough like `Entity` that
driving the field with two lines is correct. **Single Responsibility (S)**
makes it possible: each class knows only its own Update/Draw. And **Open/Closed
(O)** is what comes next.

---

## Part 6 — O, L and D, felt in your hands

### Open/Closed: add an enemy without editing the loop

Write a subclass that changes only the movement:

```cpp
// ZigzagEnemy.h — finished version
#pragma once
#include "Enemy.h"

class ZigzagEnemy : public Enemy {
public:
    ZigzagEnemy(Texture2D texture, Vector2 start, int health, float fallSpeed)
        : Enemy(texture, start, health, fallSpeed), fallSpeed(fallSpeed),
          moveRight(GetRandomValue(0, 1) == 1) {}

    void Update(float dt) override {
        position.y += fallSpeed * dt;
        position.x += 60.0f * dt * (moveRight ? 1.0f : -1.0f);
        if (position.x < 0.0f) moveRight = true;
        if (position.x > 760.0f) moveRight = false;
    }

private:
    float fallSpeed;
    bool moveRight;
};
```

Note we *added our own* `fallSpeed` / `moveRight` rather than tapping into
`Enemy`'s private `speed` — encapsulation keeps a child from poking into the
parent's guts. (A cleaner version would give `Enemy` a `protected` speed, but
that is a design taste you can explore later.)

Now spawn plain `Enemy` or `ZigzagEnemy` — the **game loop does not change one
line**; it still says `thing->Update()`. That is **O**: closed to modification
(you never edit the loop for a new kind) and open to extension (you just add a
class). Before SOLID, a new enemy meant editing `UpdateEnemies` and adding a
branch.

**L in action:** `ZigzagEnemy` lives happily in `vector<unique_ptr<Entity>>`
because it obeys the `Entity` contract. If its `Draw` depended on something a
plain `Enemy` didn't provide, the loop would break — that is the Liskov
violation to avoid.

**D in action:** the loop deals with `Entity`, never naming `ZigzagEnemy`. The
high-level Game depends on the abstraction, not the concrete class.

---

## Part 7 — The wall: collision is *not* a single-class concern

Now the honest part — and the true reason abstraction "never fit" before.

Collision is the one place "just make it a base class" **does not** cleanly
work. A collision is an *interaction between two things of different kinds*.
`Entity` can give you the rectangle, but the **effect** (bullet → enemy takes
damage; enemy → player loses a life) depends on the exact pair of kinds.

The cleanest, engine-honest solution is a **type tag** + `static_cast`:

```cpp
void Game::ResolveCollisions() {
    for (auto &a : things) {
        for (auto &b : things) {
            if (a.get() == b.get()) continue;                    // same object
            if (!a->active || !b->active) continue;
            if (a->kind() == EntityKind::Bullet &&
                b->kind() == EntityKind::Enemy) {
                Enemy &enemy = static_cast<Enemy &>(*b);
                enemy.TakeDamage(static_cast<Bullet &>(*a).damage);
                a->active = false;                // bullet is consumed
            } else if (a->kind() == EntityKind::Enemy &&
                       b->kind() == EntityKind::Player) {
                static_cast<Player &>(*b).TakeDamage(1);
                a->active = false;
            }
        }
    }
}
```

Three rules that keep this from becoming a kitchen sink:

1. **Use the `kind()` tag, not `dynamic_cast`.** The tag already tells you the
   type, so `static_cast` is correct, fast and obvious. `dynamic_cast` is slower
   and lures you into a messy catch-all.
2. Handle only the pairs that matter (here: bullet-vs-enemy, enemy-vs-player);
   everything else is ignored, cheaply.
3. This function only **marks** (`active = false`) — it never erases while
   iterating. Removal happens afterwards (see Mistake 5).

**Collision is legitimately where an abstraction ends.** Real engines keep
"which kinds interact" (a table/tag) separate from "how an object moves" (the
class hierarchy), and that separation is the pay-off of **S**. And it is the
true reason "abstract class" felt impossible earlier: Update/Draw are easy, but
the damage is a relationship between two concrete types — it cannot live inside
one base class.

---

## Part 8 — The thin-base rule (I)

The whole base, for the record:

```cpp
class Entity {
public:
    Vector2 position{};
    Vector2 size{};
    bool active = true;

    virtual ~Entity() = default;
    virtual void Update(float dt) = 0;
    virtual void Draw() const = 0;
    virtual Rectangle bounds() const;
    virtual EntityKind kind() const = 0;
};
```

That is it: data + three virtuals. **Nothing about bullets, nothing about
enemies.** The moment you pull Player-only or Enemy-only fields up here "to make
it easier", you have broken **I**:

- the base must NOT hold `health` (a Power-Up / Bullet would have to fake it);
- must NOT hold `texture` (each child owns its own draw identity);
- must NOT hold `speed` (a static Power-Up has no speed).

If only *some* children need a member, the base is not its place. Ask instead
"who really needs it?" and give it at the right level — a small seam, not a
fat base.

---

## Part 9 — `main.cpp` shrinks to three honest lines

After all of that, `main.cpp`'s only job is "start the game". This is **S** at
its biggest level:

```cpp
#include "Game.h"

int main() {
    Game game;     // loads textures, builds the player, in the constructor
    game.Run();    // the whole loop: window, update, draw, collisions
    return 0;
}
```

The `Game` class owns the window, the textures, the spawner timers, and the
field. It no longer names every variable and type each frame — it tells the
family to Update and Draw.

---

## The files after

```
space-shooter/
├── main.cpp            ← 3 lines: make a Game, run it
├── Game.h/.cpp         ← window, textures, spawner, loop, collisions
├── Entity.h/.cpp       ← abstract base (kind, bounds, update, draw)
├── Player.h/.cpp       ← concrete
├── Enemy.h/.cpp        ← concrete, base of ZigzagEnemy
├── ZigzagEnemy.h/.cpp  ← new, from Enemy (O / L demo)
├── Bullet.h/.cpp       ← concrete
├── Sprite.h/.cpp       ← GetFrameSize  (from ch07)
├── HUD.h/.cpp
└── Makefile            ← no change (wildcard *.cpp)
```

---

## Common Mistakes

### Mistake 1 — Abstraction theatre
Make `Entity`, but every caller still writes `if (type == "Zigzag")`. If you
keep name-checking concrete types all over, the abstraction gave you nothing.
Drive the loop through the interface; route only genuine collision-pairs (by
tag) outside it.

### Mistake 2 — Missing the virtual destructor
`std::unique_ptr<Entity>` calls `delete thing` where `thing` is an `Entity*`.
Without a virtual destructor on the base, deleting a `Bullet` that way is
undefined behavior. Rule: **any polymorphic class gets a virtual destructor.**

### Mistake 3 — A fat base class
Pulling `health`, `texture`, `speed` up into `Entity` "so it's easier" forces
every child to drag along fields it never uses. Keep it lean (the Part 8 rule).

### Mistake 4 — `dynamic_cast` everywhere
Prefer the `kind()` tag + `static_cast` for collisions. It reads clearly and is
fast. `dynamic_cast` is legitimate in rare cases; as a habit it is a smell.

### Mistake 5 — Erasing while iterating
Removing an item from a `vector` mid-loop shifts everything and you walk off the
end → crash. Pattern: **mark** (`active = false`) during the loop, **erase**
afterwards:

```cpp
things.erase(
    std::remove_if(things.begin(), things.end(),
                   [](const std::unique_ptr<Entity> &e) { return !e->active; }),
    things.end());
```

---

## Homework

Build and run after each one.

**Homework 1 — Smallest taste first.** Make a tiny scratch file: `Entity` with
`class Square : Entity` and `class Circle : Entity`, a `vector<unique_ptr<Entity>>`,
and the two-line Update/Draw loop. Confirm `Entity e;` won't compile — that is
your abstract class working. This single exercise makes "abstract class" stop
scaring you.

**Homework 2 — The big migration (the whole chapter).** Move Player, Enemy,
Bullet onto the `Entity` base; give `Game` the loop, the spawners and the
collision pass; shrink `main.cpp` to 3 lines. Gameplay identical. Commit at the
moment it compiles so you can diff.

**Homework 3 — Feel Open/Closed.** Add `ZigzagEnemy`. Spawn a mix of `Enemy`
and `ZigzagEnemy`. The loop must not change. If you had to edit `Game` to fit
it in, you introduced a leak (usually a fat base or a hard-coded cast).

**Homework 4 — Feel Liskov.** Add `class ArmoredEnemy : public Enemy` that
ignores damage for a couple of seconds after the first hit (an `armorTimer`,
checked with `GetTime()`). `ResolveCollisions` still uses only `Enemy`. That is L:
swap in new kinds freely and the loop stays correct.

**Homework 5 — The I promise.** Add `class PowerUp : public Entity` (e.g. a
shield that fades over time). It needs *no* health. If you catch yourself adding
`health` to the base "just for the Power-Up", you are breaking I — keep the
base lean.

---

## Next

**Chapter 08 — Sound & Music.** Now that entities sit behind a single
interface, "adding things that move and draw" is cheap. Next: "things that make
sound" — `InitAudioDevice`, `LoadSound`, `PlaySound`, `LoadMusicStream`, and an
explosion SFX triggered from the collision pass when an entity dies.