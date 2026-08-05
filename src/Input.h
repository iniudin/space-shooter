#pragma once

#include "raylib.h"
enum class InputAction {
    CONFIRM,
    CANCEL,
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT,
};

KeyboardKey KeyFor(InputAction action);

bool IsActionPressed(InputAction action);

bool IsActionDown(InputAction action);
