cmake_minimum_required(VERSION 3.16)
project(SpaceShooter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# raylib via Homebrew (macOS)
find_path(RAYLIB_INCLUDE_DIR raylib.h PATHS /opt/homebrew/include)
find_library(RAYLIB_LIBRARY raylib PATHS /opt/homebrew/lib)

add_executable(main
        src/main.cpp
        src/Player.cpp
        src/Enemy.cpp
        src/Bullet.cpp
        src/Sprite.cpp
        src/HUD.cpp
        src/Entity.h
        src/Config.h
        src/Input.h
        src/Input.cpp
        src/Game.h
        src/Game.cpp
)

target_include_directories(main PRIVATE src ${RAYLIB_INCLUDE_DIR})
target_link_libraries(main PRIVATE ${RAYLIB_LIBRARY})
