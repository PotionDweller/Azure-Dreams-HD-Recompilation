# Findings: what does what

Reverse-engineering notes for Azure Dreams (USA, SLUS-00614): where things
live in the game, what we change, and how each one was found. Addresses are
PlayStation RAM addresses (`0x80xxxxxx`). "Overlay" means code the game loads
at runtime into a shared region, so the same address can hold different code
in town and in the tower; hooks check stock values before changing anything.

## Quick reference

| What | Where | Stock | We change | Lives in |
|---|---|---|---|---|
| Terrain view rectangle (decides which tiles/walls are drawn) | `0x80083178` x, `0x8008317C` w | x = -188, w = 370 | x - m, w + 2m | `src/mod_widescreen.c` |
| Sprite on-screen test (characters, monsters) | 12 `sltiu x+32, 385` sites, see below | 385 | widened by margin | `game.toml` `[widescreen.cull]` |
| GTE screen-extent rejects | immediates `0x181` (385) / `0x121` (289) | 320x224 + 32 px | widened by margin | `game.toml` `[widescreen.cull]` |
| Left HUD panel (LV / HP) anchors | `0x800E2924`, 4 x `{int16 x, int16 y}` | x {-196, -140, -90, -34}, y {-138, -86, -138, -82} | x - m | `src/mod_widescreen.c` |
| Dungeon minimap routine | `0x8009E798` (dungeon overlay) | prologue `0x27BDFFB8`, `+8` `0x00A0B021` | hooked at entry | `src/mod_widescreen.c` |
| Minimap clip window | `$a0 + 4` RECT, copy of x at `$a0 + 12` | {213, 56, 107, 112} | → {192 + m, 56, 128, 128} | `src/mod_widescreen.c` |
| Minimap content anchor | `$a1 + 2` (x), `$a1 + 6` (y) | 256, 120 | x + m | `src/mod_widescreen.c` |
| Controller type | `game.toml` `[controller]` | — | `lock_mode = true` (digital) | `game.toml` |
| Fast Loading engage | framework `main.cpp` | 4 frames | 90-frame CD burst; bulk memory-card transfers | `patches/` |

`m` is the widescreen reveal margin: 53 px per side at 16:9 (426 x 240 view),
0 at 4:3, so every change is inert with widescreen off.

## Screen extension (16:9)

- **Rendering:** the framework's native-wide mode draws a 426-px-wide view at
  the original proportions. Enabled by the `azuredreams.widescreen` mod via
  `psx_mod_set_fixed_display_aspect(16, 9)`; `[widescreen] gte_game_mode`
  engages it only during gameplay, so movies, menus and the BIOS stay 4:3.
- **Terrain pop-in (diagonal `\ /` edges):** the town/tower renderer does not
  cull tiles against the screen. `func_80046884` takes the view rectangle
  (camera `0x80083160` + 24), casts its four corners onto the ground plane,
  and `func_80046C20` scan-converts that footprint into the tiles to draw. The
  overlays set the stock rectangle once per area, so the mod re-widens it every
  VBlank while it still holds stock values. A third caller (`0x800CF9A4`, a
  local rectangle with h = 442) is **not** widened; untested where it is used.
- **Sprite pop-in at the edges:** characters and monsters use a biased
  4-corner test `sltiu x+32, 385` in `func_80037714` (main EXE) and its overlay
  copy `func_800AFFB4`. Sites: `0x800377F0 0x80037820 0x80037854 0x80037888
  0x80037A90 0x80037AC0 0x80037AF4 0x80037B28 0x800B00BC 0x800B00EC 0x800B0120
  0x800B0154`. The paired `sltiu y+32, 289` compares are vertical and left
  alone. Found with `psxrecomp-analyze --scan-widescreen`.
- **Not a width:** `0x1D6` (470) in `func_800453E0` bounds an object countdown
  at `obj+192`; widening it breaks gameplay timing.
- **White wall strips / invisible NPCs after a new game (fixed):** before each
  movie the game sets the draw area to all of VRAM and clears 480x448 at (0,0)
  with a black GP0(60) rect, then reloads the textures it covered. The
  framework's native-wide mode widened full-display rects by 53 px on both
  sides in VRAM itself, so the clear also zeroed x 480–532: a town house's wall
  texture (tpage x 512) and NPC sprites such as Wreath (tile 480,384), which
  the game never reloads. The patch in `patches/` drops that widening; the
  renderers already extend full-display rects across the wide surface. Loading
  a save never hit it (no movie on that path).
- **Top/bottom tiles at the highest camera:** the edge of the drawn area can
  show at the top and bottom of the screen at the highest camera angle in
  dungeons. Identical with widescreen off, so it is the game's own draw
  radius, not a port bug. Widening it would be a new enhancement.

## Moving the UI

- **HUD panel (top-left):** drawn through `func_8004C36C` with an identity
  "screen" camera, positioned by the anchor table at `0x800E2924`. Entry 0 is
  the HP thermometer, normally parked just off-screen. Shifting all four x
  values by `-m` puts the panel on the wide left edge.
- **Dungeon minimap (right):** `func_8009E798` is reached through a function
  pointer, and its structs are heap-allocated and move between floors, so the
  mod hooks the function entry (`[recompiler] mod_function_entry_funcs`) and
  reads them from the arguments: `$a0` holds the clip window, `$a1` the map
  state. Both x values move by `+m`. The window is a **draw-area clip inside
  the framebuffer**, which the framework's native-wide mode did not mirror; the
  renderer patch in `patches/` adds that. Stock, the window reaches only 43 px
  left of and 48 px below the map anchor (64 px right/up), cutting off larger
  maps, so the mod also grows it to 64 px on every side. At the most zoomed-out level the map
  is small and centred in its window, so it does not touch the edge; that is
  the game's own layout.

## Speed-ups

- **Black screen at ~7 s (fixed):** after seeking to the intro movie
  (`/STR/KONAMI.STR`, LBA 87535) the game polls Getstat; the framework
  reported the Read bit on SeekL completion (a Gran Turismo workaround) and the
  game waited forever. Fixed in `cdrom.c`.
- **Loads:** framework Fast Loading (host pacing) runs the game uncapped while
  a load is detected. Guest timing is unchanged, so nothing can desync.
- **Saves:** in-game saves/loads are almost entirely memory-card transfers
  (~15–22 s originally). The patch counts **bulk** card transfers (writes, and
  reads of sectors not read in the last ~4 s of guest time) as a load. Save
  menus re-read the same directory sectors, so they stay at normal speed.
  Result: load ~3 s, save ~2 s.
- **In-game lurch (fixed):** NPC portraits, equipping gear, the formation menu
  and summoning read 1–8 sectors over 1–4 frames. The framework's 30-frame
  burst tail kept turbo on afterwards and fast-forwarded ~0.5 s of play. A CD
  read now only counts after 90 frames of continuous reading. Measured: real
  loads 435–495 frames (848–970 sectors), largest in-play read 71 frames.
- **Sped-up load-in music (fixed):** area music often starts while its load is
  still reading. Uncapped Fast Loading now discards accelerated audio, and the
  first SPU key-on during an accelerated disc load ends acceleration for the
  rest of that load, so the music plays from its first note at real speed.
  Memory-card saves/loads keep full speed. `turbo_loads` lists recent loads
  (start frame, music key-on frame, end frame, accelerated frames).

## Other facts

- **Boot EXE:** `SLUS_006.14`, 512 KB on disc but its header declares 348,160
  bytes; the recompiler needs the exact size, so the patched prepare step trims the extracted copy automatically.
  Load address `0x8002D000`, entry `0x80033930`.
- **Controller:** the game predates the DualShock and ignores a pad in analog
  mode (id `0x73`), so `lock_mode = true` pins every seat to digital.
- **PGXP:** the framework's PGXP mod is the single switch for geometry
  correction; `game.toml` leaves it off so the Mods toggle really disables it.
- **Movies** (`/STR/`, start LBA): KONAMI 87535, LOP1_WS 49231, LOP2_WS 64255
  (opening), JO001_WS 38340, JO002_WS 39501, JO003_WS 41493, TOP_WS 86585,
  STAFROLL 88956. Shown via the 24-bit display path, upscaled with
  `fmv_filter = "bicubic"`.
- **Japanese release:** has full voice acting and a different title screen and
  art ([TCRF](https://new.tcrf.net/Azure_Dreams_(PlayStation))). All addresses
  above are for the US version and would have to be re-found there.

## Potential future goals

- **Japanese version with voice acting and an English translation.** The
  Japanese release (*Other Life: Azure Dreams*, Nov 1997) has voiced dialogue
  for every character and intro narration, all removed from the international
  versions ([TCRF](https://new.tcrf.net/Azure_Dreams_(PlayStation))). The goal
  is to recompile the Japanese version itself, apply an English translation,
  and bring over this port's enhancements (widescreen, HUD and minimap moves,
  fast loading, PGXP and the framework patches). Open questions:
  - Each player would need their own Japanese disc.
  - Whether a complete English fan translation of the Japanese version exists
    is not yet known.
  - Every address in this document is for the US version and would have to be
    found again in the Japanese one; the sections above are the checklist.
  - Reports that the US release is censored compared to the Japanese one are
    unconfirmed; concrete examples are welcome.

## How things were found

The debug build (`--debug-port 4370`, client `psxrecomp/tools/debug_client.py`)
exposes the running game. Most useful commands:

- `gpu_frame_dump` — every GPU command in a frame, including draw-area (E3/E4)
  changes; found the minimap clip window.
- `fntrace_arm` / `fntrace_dump` — arguments of a function on entry; found the
  minimap structs.
- `wtrace_add` / `wtrace_dump` — which code writes a RAM range; used to trace
  where on-screen positions come from.
- `read_ram` / `write_ram` — inspect and nudge values live.
- `cdrom_bursts` — length and sector count of every disc read; tuned Fast
  Loading.
- `screenshot` — in widescreen writes the full 16:9 frame to `psx_wide.png`.
- `vram_upload_ring` / `vram_upload_get` — recent CPU→VRAM uploads with the
  pixels the game sent; showed the house and Wreath textures arrived intact.
- `gl_fbo_peek` — read the renderer's VRAM for a rect; sampled repeatedly it
  caught the moment those textures were wiped. (It packs the rect as it reads,
  so the probe's own rects show up in `gl_coh_ring`.)
