# Sly Cooper: Thieves in Time — Custom Debug Menu

A reverse-engineered, from-scratch recreation of Sly 4's internal developer debug
menu, built via SPRX injection on PS3 (RPCS3 and real devkit/modded console).

## Features

- **Tunable browser** — navigate the game's internal tunable registry by dotted
  category path (e.g. `Camera.Default`, `Game.Player`), with live editing of
  bools, ints, floats, vectors, colors, and strings.
- **Player tools** — live Coins counter, live Position (X/Y/Z) editor.
- **Fly mode** — free-flying movement with vertical control, gravity/flailing
  suppressed.
- **Episodes browser** — every episode in the game, names resolved live from
  the game's own localization system.
- **Jobs browser** — every job (mission) in each episode, with direct warp
  support: jump straight into any mission, or send yourself to that episode's
  hub/hideout instead.
- **Goal browser** — every checkpoint/goal within a job, color-coded
  (green/yellow/red for done/current/upcoming), with click-to-warp support.
- **Save/load system** — persist your favorite tunable overrides to disk.

## Controls

| Button | Action |
|---|---|
| **D-Pad Up/Down** | Move selection up/down in the current menu |
| **D-Pad Left/Right** | Adjust the selected value (hold for fast repeat) |
| **L1 (held) + Left/Right** | Adjust value by a larger step (×10) |
| **Cross (X)** | Select / enter category / toggle bool / activate action |
| **Square** | Go back one menu level (or close the menu from the top level) |
| **Circle** | Toggle "saved" status on the selected tunable (persists to disk) |
| **Left Stick** | Drag/reposition the menu on screen |
| **L2 + X** (on a job entry) | Warp to that episode's hub, without starting the job |
| **R2 + X** (on a job entry) | Warp to that episode's hideout, without starting the job |
| **X** (on a job entry, no L2/R2) | Warp directly into that mission |
| **X** (on a goal entry) | Warp directly to that specific checkpoint within the active mission |

## Menu Structure

[Debug Menu]

├── Tuning...        → full tunable registry browser
├── Anim Preview Mode... (not implemented)
└── Load...
├── Levels...     (not implemented)
├── Episodes...   → episode list → job list → goal list
└── Save/Load...  (not implemented)

Inside the Tuning browser, two synthetic categories are injected alongside
the real tunable tree:
- **Menu** — debug menu's own settings/actions (background color, fly mode
  toggle, save file management, vtable dump utilities)
- **Player** — Coins, Position
- **Episodes** — the episode/job/goal browser described above

## Installation

### Requirements
- Sly Cooper: Thieves in Time (PS3, Title ID **BCES01284**)
- A way to load a custom SPRX into the game process — either:
  - **RPCS3** with a debug build/plugin loader, or
  - A **modded retail console** (CFW) with a plugin/payload loader

### Steps
1. Build the SPRX using the PS3 SDK toolchain (SNC C++11 / Visual Studio 2013
   PS3 project).
2. Place the built `.sprx` in your game's plugin-load path:
   - **RPCS3**: typically alongside other loaded plugins per your loader setup
   - **CFW/modded console**: in your plugin loader's designated SPRX directory
   for title `BCES01284`
3. Launch the game. The debug menu hook installs automatically on boot.
4. Default toggle: **[fill in your actual menu-open button combo here]**

### Save File Location
Tunable overrides marked with Circle are saved to: /dev_hdd0/tmp/Sly4DebugMenu/debugmenu.cfg

## Known Limitations
- Switching directly between two different missions *within the same episode*
  while one is already active will redirect you to the hideout first — select
  the mission again from there to start it cleanly.
- Camera-relative fly movement is not implemented; fly mode uses world-axis
  movement only.

## Credits
Built through reverse engineering via IDA Pro (cross-referenced against the
Android port for symbol recovery), live debugging via RPCS3/devkit, and a lot
of trial and error.
