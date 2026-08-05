#include "Input.h"
#include "raylib.h"

KeyboardKey KeyFor(InputAction action) {
    switch (action) {
        case InputAction::CONFIRM:
            return KEY_X;
        case InputAction::CANCEL:
            return KEY_C;
        case InputAction::MOVE_UP:
            return KEY_UP;
        case InputAction::MOVE_DOWN:
            return KEY_DOWN;
        case InputAction::MOVE_LEFT:
            return KEY_LEFT;
        case InputAction::MOVE_RIGHT:
            return KEY_RIGHT;
    }
    return KEY_NULL;
};

bool IsActionPressed(InputAction action) { return IsKeyPressed(KeyFor(action)); };

bool IsActionDown(InputAction action) { return IsKeyDown(KeyFor(action)); }
