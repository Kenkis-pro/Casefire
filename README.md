# CASEFIRE

CASEFIRE is a small first-person 3D shooter written from scratch in C++17 with OpenGL 3.3, GLFW, glad, GLM, nlohmann/json, and OpenAL. It intentionally avoids full game engines and keeps rendering, gameplay, shop, inventory, admin controls, and generated beep sounds in simple C++ modules.

## Features

- Minimal 3D arena with a flat floor and cube cover walls.
- First-person movement with `WASD`, mouse look, and left-click shooting.
- Red cube enemies with 3 HP that chase the player and deal 10 contact damage.
- Player HP, Game Over state, and restart with `R`.
- Coin reward: every killed enemy grants 10 coins.
- Main menu and keyboard-driven shop:
  - `ENTER` starts the game.
  - `S` opens the shop from the main menu.
  - `O` opens a case for the configured case price.
  - `E` cycles owned weapon skins.
  - `X` sells the equipped non-default skin for 50% of its configured price.
  - `B` returns to the main menu.
- JSON persistence:
  - `profile.json`: coins, health, ban flag, equipped skin.
  - `inventory.json`: owned skin ids.
  - `skins.json`: skin id, name, RGB color, price, drop chance.
  - `case_config.json`: case price.
  - `admin.json`: admin-panel enable flag.
- F1 admin panel, enabled only when `admin.json` contains `"admin": true`:
  - Add coins.
  - Grant a skin by id.
  - Change case cost.
  - Change skin drop chances.
  - Ban/unban the player.

## Dependencies on Arch/CachyOS

```bash
sudo pacman -S --needed base-devel cmake glfw-x11 glad glm nlohmann-json openal
```

If your setup uses Wayland-native GLFW instead of X11, replace `glfw-x11` with the GLFW package variant used by your distribution.

## Build and run

```bash
cmake -S . -B build
cmake --build build -j$(nproc)
./build/casefire
```

The game writes JSON save/config files into the current working directory. Run the executable from the repository root if you want those files next to this README.

## Controls

| Context | Key / Mouse | Action |
| --- | --- | --- |
| Main menu | `ENTER` | Start match |
| Main menu | `S` | Open shop |
| Any state | `F1` | Open admin panel if `admin.json` has `admin: true` |
| Any state | `ESC` | Quit |
| Playing | `WASD` | Move |
| Playing | Mouse | Look around |
| Playing | Left mouse button | Shoot |
| Game Over | `R` | Restart match |
| Shop | `O` | Open case |
| Shop | `E` | Equip next owned skin |
| Shop | `X` | Sell equipped non-default skin |
| Shop | `B` | Back to main menu |

## Notes

- The window title displays the current HP, coins, enemy count, shop status, or Game Over/Banned state. This avoids adding a font dependency while still keeping the project minimal and self-contained.
- Sounds are generated at runtime with OpenAL sine-wave buffers: shot, hit, case open, and error.
- Skin drop chances are interpreted as percentages; use the admin panel to keep their sum at 100 for predictable rolls.
