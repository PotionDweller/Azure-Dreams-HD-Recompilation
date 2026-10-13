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

> These steps describe how this port was built. They have not yet been tested
> from a fresh clone on another machine; please open an issue if a step fails.

Requirements: Git, Python 3, and your own `Azure Dreams (USA).cue/.bin`
(the cue/bin SHA-1s are listed in `game.toml` under `[prepare_disc]`).
The C toolchain (clang, CMake, Ninja, SDL3) is downloaded by psxrecomp.

```powershell
git clone --recursive https://github.com/PotionDweller/Azure-Dreams-HD-Recompilation.git AzureDreamsRecomp
cd AzureDreamsRecomp

# 1. Apply this project's framework fixes (see patches/README.md).
powershell -ExecutionPolicy Bypass -File scripts\apply-psxrecomp-patch.ps1

# 2. Download the toolchain and generate the game C from YOUR disc.
python psxrecomp\psxrecomp_cli.py ensure-toolchain
python psxrecomp\psxrecomp_cli.py generate --config game.toml --project-root . --disc "C:\path\to\Azure Dreams (USA).cue"

# 3. Build.
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target psx-runtime
```

Then run `build-release\AzureDreams_Recompiled.exe`. The launcher opens; pick
your disc with **Browse For Disc** the first time, then press **PLAY**.

**Known gotcha:** the boot executable on the disc (`SLUS_006.14`) is 512 KB,
but its header declares 348,160 bytes and the recompiler requires the exact
size. If generation fails on the EXE size, truncate the extracted
`disc\SLUS_006.14` to 348160 bytes and run `generate` again
(the BIOS only ever loads the declared size).

### Overlays (optional, for speed)

Azure Dreams loads extra code ("overlays") at runtime. Out of the box those run
in the framework's interpreter, which works but is slower. To compile them:
set `overlay_cache = true` (already on in `game.toml`), play through the areas
you care about, close the game normally, then run
`psxrecomp\tools\compile_overlays.py --static` and rebuild. Captures contain
game code and are gitignored; each player makes their own.

## Repository layout

| Path | What |
|---|---|
| `game.toml` | Game config: identity, video defaults, widescreen cull sites, controller |
| `src/mod_widescreen.c` | Widescreen mod plugin (view rectangle, HUD, minimap) |
| `mods/preloaded/packages/` | Mod packages shown in the launcher (widescreen; default-on overrides of Fast Loading and PGXP) |
| `patches/` | Fixes applied to the pinned psxrecomp framework |
| `psxrecomp/`, `recomp-ui/` | Framework and launcher submodules (pinned) |

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
