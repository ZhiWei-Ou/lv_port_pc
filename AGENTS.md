# Repository Guidelines

## Project Structure & Module Organization

This repository is a desktop simulator for developing custom LVGL UI components with SDL2.

- `src/main.c` initializes the 700 × 700 window and launches the demo selected by CMake; `src/hal/` handles display and input setup.
- `components/` contains reusable widgets. The existing `glow/` component keeps its demonstration in `glow/demo/`.
- `src/mouse_cursor_icon.c` contains the cursor image asset.
- `lv_conf.h` controls LVGL features; `config/FreeRTOSConfig.h` configures optional FreeRTOS support.
- `lvgl/` and `FreeRTOS/` are upstream submodules. Keep project-specific changes outside them unless dependency changes are intended.
- `build/` and `bin/` contain ignored build outputs.

## Build, Test, and Development Commands

Install a C/C++ compiler, CMake, a build tool, and SDL2 development headers (`libsdl2-dev` on Debian/Ubuntu).

```sh
git submodule update --init --recursive  # Initialize dependencies
cmake -B build -S .                     # Configure
cmake --build build -j                  # Build the component library and all demos
./bin/tracking_chart_demo                             # Launch the simulator
cmake --build build --target run_tracking_chart_demo        # Build and launch
```

For debugging, configure with `-DCMAKE_BUILD_TYPE=Debug`; add `-DASAN=ON` for AddressSanitizer with a compatible compiler. FreeRTOS requires both `-DUSE_FREERTOS=ON` and `LV_USE_OS LV_OS_FREERTOS` in `lv_conf.h`.

## Coding Style & Naming Conventions

Use C99 or C++17 as configured by CMake. Match nearby code: four-space indentation, snake_case identifiers, uppercase macros, and function opening braces on separate lines. Public component APIs use `<component>_<action>`, such as `glow_create(parent)`; keep internal helpers `static`.

Pair component `.c` and `.h` files, use header guards, and place each component in `components/<name>/`; CMake automatically discovers sources and excludes `demo/` from the component library. Demo entry points follow `<name>_demo()` in `demo/<name>_demo.h`. Keep component behavior local and screen layout in application/demo code. No repository-level formatter or linter configuration is provided.

## Testing Guidelines

No repository-level automated test suite or coverage threshold is configured. Build changes and exercise affected interactions in the simulator. For visual changes, check clipping, gradients, animation, and the FPS/memory monitors. Record reproduction steps and validation results; do not treat upstream submodule tests as application coverage.

## Commit & Pull Request Guidelines

Recent project commits use Conventional Commits, for example `feat(glow): add animated glow frame component and demo`. Use an accurate type and scope, and keep changes focused.

PRs should explain the problem and resulting behavior, list build/manual checks, link relevant issues, and include screenshots or recordings for visible UI changes. Identify configuration or dependency changes explicitly.
