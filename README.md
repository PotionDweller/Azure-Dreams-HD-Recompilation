# Azure Dreams HD Recompilation

A native Windows PC port of **Azure Dreams** (PlayStation, USA, SLUS-00614),
made by statically recompiling the original game code to C with
[psxrecomp](https://github.com/mstan/psxrecomp), with the
[recomp-ui](https://github.com/mstan/recomp-ui) launcher.

**This repository does not contain the game.** You need your own legal copy of
the disc. No game data, BIOS, or recompiled game code is distributed here.

## Features

All are toggles in the launcher (Settings / Mods). Defaults are the HD
Widescreen setup; turn everything off for the original experience.

| Feature | Default | Notes |
|---|---|---|
| 16:9 widescreen | on | Wider town and tower view at the original proportions; HUD panel and dungeon minimap moved to the wide edges. Movies and menus stay 4:3. |
| HD rendering | 4x | Internal resolution up to 4x. |
| PGXP precision | on | No polygon wobble; perspective-correct textures (separate checkbox). |
| Fast loading | on (uncapped) | Includes in-game saves and loads: about 2–3 s instead of 17–22 s. |
| Movie filter | bicubic | Smoother upscaling of movies and the title art. |
| Borderless fullscreen | on* | Alt+Enter toggles a maximised window; Alt+F4 quits. |

\* Set by the portable package; a fresh source build starts windowed.

Controllers: any SDL gamepad or the keyboard, in the original digital-pad mode
(the game predates the DualShock and ignores analog-mode pads).

## Building (Windows)

> Tested from a fresh clone on Windows 10 with only Git and Python installed
> (the framework fetched its own toolchain). Please open an issue if a step
> fails for you.

Requirements: Git, Python 3, and your own `Azure Dreams (USA).cue/.bin`
(the cue/bin SHA-1s are listed in `game.toml` under `[prepare_disc]`).
The C toolchain (clang, CMake, Ninja, SDL3) is downloaded by psxrecomp; you do
not need CMake installed.

**Clone into a short folder** such as `C:\src`. The framework has deeply nested
files, and a long path fails on Windows with "Filename too long". Alternatively
run `git config --global core.longpaths true` first.

```powershell
cd C:\src
git clone --recursive https://github.com/PotionDweller/Azure-Dreams-HD-Recompilation.git AzureDreamsRecomp
cd AzureDreamsRecomp

# 1. Apply this project's framework fixes (see patches/README.md).
powershell -ExecutionPolicy Bypass -File scripts\apply-psxrecomp-patch.ps1

# 2. Download the toolchain and generate the game C from YOUR disc.
python psxrecomp\psxrecomp_cli.py ensure-toolchain
python psxrecomp\psxrecomp_cli.py generate --config game.toml --project-root . --disc "C:\path\to\Azure Dreams (USA).cue"

# 3. Build (uses the framework's own toolchain).
python psxrecomp\psxrecomp_cli.py rebuild --config game.toml --project-root . --build-dir build-release --target psx-runtime --no-pgo
```

Then run `build-release\AzureDreams_Recompiled.exe`. The launcher opens; pick
your disc with **Browse For Disc** the first time, then press **PLAY**.

### Overlays (optional, for speed)

Azure Dreams loads extra code ("overlays") at runtime. Out of the box those run
in the framework's interpreter, which works but is slower. To compile them:
set `overlay_cache = true` (already on in `game.toml`), play through the areas
you care about, close the game normally, then run
`psxrecomp\tools\compile_overlays.py --static` and rebuild. Captures contain
game code and are gitignored; each player makes their own.

### Original or HD

`presets/` holds two ready-made setups, picked with `--preset`:

- `--preset hd`: widescreen, 4x resolution, perspective-correct textures,
  smoothed movies, Fast Loading and 150% CPU (no slowdown).
- `--preset original`: plays like the PlayStation: 4:3, native resolution,
  original loading and speed.

Each keeps its own settings and mod choices in `presets/<name>/` next to the
exe; the disc choice, controls and memory cards are shared. Add
`--no-launcher` to skip the launcher.

### Release package

`scripts\package-release.ps1 -Zip` turns a Release build into
`dist\AzureDreams-HD-Recompiled-<VERSION>.zip`: the exe, OpenBIOS, mods, both
presets with a `.bat` launcher each, and a player README. It contains no game
data (the script refuses to package disc images, saves, captures or box art);
players pick their own disc image on first launch.

## Findings

Everything discovered while making this port is written up in
[`docs/FINDINGS.md`](docs/FINDINGS.md), and you are welcome to use it for
your own experiments, mods or ports: the memory addresses and original values
behind the widescreen view, the HUD and minimap positions, the screen-edge
checks and the loading speed-ups, plus the debug commands used to find them.

## Repository layout

| Path | What |
|---|---|
| `game.toml` | Game config: identity, video defaults, widescreen cull sites, controller |
| `src/mod_widescreen.c` | Widescreen mod plugin (view rectangle, HUD, minimap) |
| `mods/preloaded/packages/` | Mod packages shown in the launcher (widescreen; default-on overrides of Fast Loading and PGXP) |
| `presets/` | The Original and HD setups (`--preset original` / `--preset hd`) |
| `scripts/package-release.ps1` | Builds the release zip (no game data) |
| `patches/` | Fixes applied to the pinned psxrecomp framework |
| `docs/FINDINGS.md` | **What does what:** addresses and values behind widescreen, the HUD/minimap moves and the speed-ups, and how they were found |
| `psxrecomp/`, `recomp-ui/` | Framework and launcher submodules (pinned) |

## Credits

- **[psxrecomp](https://github.com/mstan/psxrecomp)** and
  **[recomp-ui](https://github.com/mstan/recomp-ui)** — the static
  recompilation framework and launcher this port is built on.
- **OpenBIOS** — the bundled open-source PlayStation BIOS.
- **[libretro-thumbnails](https://github.com/libretro-thumbnails/libretro-thumbnails)** — optional launcher box art.

This port was made with the assistance of **Claude** (Anthropic's AI
assistant, via Claude Code). Claude helped with:

- **Debugging** — tracking down framework issues such as the black screen
  before the intro movie, controller input, fullscreen hotkeys and Fast
  Loading behaviour, using the framework's debug server and logs.
- **Reverse engineering** — tracing the game's code and memory while it runs
  to find what the enhancements needed: the camera view rectangle that decides
  which terrain is drawn, the HUD position table, the dungeon minimap's
  drawing routine and clip window, and the screen-edge culling checks.
- **Writing code** — the widescreen mod, the framework patches in `patches/`,
  and the build and packaging scripts.

All changes were tested in-game by the project owner.

If you take issue with AI-assisted recompilations, that's fine: you're free
to use something else or make your own.

## Legal

You must own the original game. Disc images, BIOS dumps, saves, overlay
captures and generated C are gitignored and must never be committed. The
bundled OpenBIOS is used by default. Azure Dreams is © Konami; this project is
not affiliated with or endorsed by Konami.

Optional box art (`launcher_assets/img/`, not included) can be fetched from
[libretro-thumbnails](https://github.com/libretro-thumbnails/libretro-thumbnails)
with `psxrecomp\tools\new_project_layout\fetch_boxart.py`; see `BOXART_SOURCE.txt`.

## Symbols

Progressive map: `symbols.toml` → `python tools/sync_symbols.py` →
`psx_symbols.h` (`PSX_FN_*`). See `psxrecomp/docs/SYMBOLS.md`.
