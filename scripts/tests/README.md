# scripts/tests/

Manual dev harnesses — **not reachable from the main menu**.

These scripts test isolated engine systems. They are loaded directly via
`-DGAME=<name>` at CMake configure time, bypassing `main_menu.lua`.

## Running a test

```bash
# 1. Configure with the test entry point
cmake -B build -DGAME=scripts/tests/sprite_test.lua

# 2. Build
cmake --build build -j

# 3. Run
./build/2d-engine
```

Replace `sprite_test.lua` with any file below. ESCAPE quits.

## Available tests

| File | What it tests | Controls |
|---|---|---|
| `sprite_test.lua` | Texture loading, `AnimationSystem`, sprite tinting (R/G/B, alpha fade) | ESCAPE to quit |
| `audio_test.lua` | SFX playback, music streaming, OGG decode, volume control | SPACE laser · F explosion · P pause/resume music · UP/DOWN volume · ESCAPE quit |

## Adding a new test

1. Create `scripts/tests/my_test.lua`.
2. Add a row to the table above.
3. Do **not** add it to `scripts/main_menu.lua` — test harnesses are dev-only.
