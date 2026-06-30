# Sly Cooper: Thieves in Time — Custom Debug Menu

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
| **L3 + START** | Open/Close Debug Menu. |
| **L2 (held)** | In Fly Mode press and hold L2 to decrease height. |
| **R2 (held)** | In Fly Mode press and hold R2 to increase height. |

### FreeCam Controls (while FreeCam is enabled)

| Button | Action |
|---|---|
| **Left Stick** | Move the camera (forward/back, strafe left/right) |
| **Right Stick** | Look around (yaw/pitch) |
| **L1 (held)** | Decrease camera height |
| **R1 (held)** | Increase camera height |
| **L2 (held)** | Move slower (precision mode) |
| **R2 (held)** | Move faster |

Player movement and controller input are automatically disabled while FreeCam
is active, and restored when it's turned off. FreeCam, along with slow-motion
and movement-speed options, is accessible from the **FreeCam** category in
the Tuning browser.

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

## Prerequisites
Visual Studio 2013+
Sony PS3 4.75+ SDK w/ Visual Studio Integration* (*not included obviously)

### Requirements
- Sly Cooper: Thieves in Time (PS3, Title ID **BCES01284**)
- A way to load a custom SPRX into the game process — either:
  - **RPCS3** or
  - A **modded retail console** (CFW) with a plugin/payload loader


To install this debug menu put the sprx file to the `/dev_hdd0/tmp/` folder and replace the **BCES01284** version's EBOOT.BIN with the one from this github page

 ## RPCS3 Configuration

Before loading the SPRX, two RPCS3 settings need to be changed from default,
or the menu (and potentially the game itself) will misbehave:

### 1. PPU Decoder → Interpreter (static)
Go to **Settings → CPU** and set **PPU Decoder** to **Interpreter (static)**.
The default PPU decoder (recompiler/LLVM) does not play well with the runtime
hooking technique this mod uses — function detours can silently fail to land
correctly, or cause crashes, under the recompiler. Interpreter mode is slower
but reliable for this kind of live code patching.

### 2. Disable "Empty /dev_hdd0/tmp/ Folder on Game Boot"
Go to **Settings → Advanced** (or **Emulator**, depending on your RPCS3
version) and find the option that empties `/dev_hdd0/tmp/` on boot — turn
this **off**.

This is required because the debug menu's save file lives at: /dev_hdd0/tmp/Sly4DebugMenu/debugmenu.cfg

If this folder gets wiped on every boot (RPCS3's default behavior for
`/dev_hdd0/tmp/`), your saved tunable overrides will never persist between
sessions — they'll be deleted the moment you relaunch the game. This setting
should be disabled **globally** (not per-game), since RPCS3 applies the
tmp-wipe behavior at the emulator level, not per-title.

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
