# psxrecomp patch

`psxrecomp-local.patch` applies to the pinned framework submodule
(psxrecomp v0.3.3-alpha, `2da25c18`). Apply it with
`scripts/apply-psxrecomp-patch.ps1`, or `git -C psxrecomp apply ../patches/psxrecomp-local.patch`.

| File(s) | Change | Why |
|---|---|---|
| `tools/prepare_disc.py` | The extracted boot EXE is trimmed to the size its header declares | Azure Dreams stores `SLUS_006.14` as 512 KB with padding; the recompiler needs the exact 348,160 bytes the BIOS loads |
| `cdrom.c` | SeekL/SeekP completion no longer reports the Read status bit | Azure Dreams polls Getstat after seeking to the intro movie and hangs on a black screen otherwise |
| `gpu.c`, `gpu_render.{c,h}`, `gpu_gl_renderer.c` | Native widescreen mirrors draws clipped to a window inside a framebuffer, and keeps the 4:3 draw inside the framebuffer | The dungeon minimap draws through such a clip window; without this it is cut off at the 4:3 edge |
| `cdrom.{c,h}`, `main.cpp` | A disc read only counts as a load for Fast Loading after 1.5 s of continuous reading | Short in-game reads (NPC portraits, equipping gear, formation menu, summoning) otherwise fast-forwarded about half a second of gameplay |
| `sio.{c,h}`, `main.cpp` | Bulk memory-card transfers count as a load for Fast Loading | In-game saves/loads take ~2–3 s instead of ~17–22 s; save menus stay at normal speed |
| `main.cpp` | `[video]` config adds to, but no longer cancels, an enabled PGXP mod | Otherwise the Mods-page PGXP switch does nothing |
| `host_keymap.c` | Hotkey modifiers match either Left or Right Alt/Ctrl/Shift | Alt+Enter and Ctrl+F never fired |
| `main.cpp` | Leaving fullscreen maximises the window | It otherwise dropped to a small fixed-size window (tiny on high-DPI displays) |
| `gpu.c` | Native widescreen no longer widens full-display flat rects in VRAM (`ws_expand_fullscreen_rect` removed; the renderers already extend them across the wide surface) | Before each FMV the game clears 480×448 at (0,0); widened to x −53..533 it wiped VRAM x 480–532, leaving white strips on a town house and invisible NPC sprites (Wreath) after a new game |
| `gpu_gl_renderer.c` | Entering 24-bit mode flushes queued uploads before discarding the queue | They were dropped from the FBO when it was clean; a later readback then lost them from CPU VRAM too |
| `main.cpp`, `spu.{c,h}` | Fast Loading discards accelerated audio when uncapped too, and the first SPU key-on during an accelerated disc load ends acceleration for that load | Area music that starts mid-load played sped up |
| `bios/OpenBIOS.toml`, `recompiler/src/{config_loader,recompiler_patch,full_function_emitter,main_bios}.*` | BIOS profiles accept `[[recompiler.patch]]` word replacements, applied to the ROM before emission (the runtime still identifies the unpatched image); OpenBIOS gets a `strrchr` fix | OpenBIOS `strrchr(s, 0)` returned 0 instead of the terminator's address; the game froze entering the tower |
| `psx_cyc{,les}.h`, `psx_cycles.c`, `psx_icache.c`, `bios_hle.c`, `data_shards.c`, `dirty_ram_interp.c`, `memory.c`, `overlay_loader.c`, `mod_plugins.h`, `mod_builtin_speed.c`, `main.cpp`, `mods/builtin/packages/psx.enhancement.cpu-overclock/` | CPU Overclock mod (`psx_mod_set_cpu_overclock`, 100–1000 %, off by default): scales only CPU-work cycle charges; `psx_advance_cycles` (idle skip, stalls) stays real-time. Changes the overlay codegen hash, so older save states are refused | Foliage/water tower floors drop to ~24 fps on original hardware; Azure Dreams enables it at 150 % (200 % made loop-timed menu cursors slippery) |
| `main.cpp`, `mod_packages.{cpp,h}`, `mod_runtime.{cpp,h}` | `--preset <name>`: settings.toml and the mod selections live in `presets/<name>/` next to the exe (disc/BIOS choice, input, mod catalog and memory cards stay shared) | One install offers both an authentic and an HD way to play (`presets/original`, `presets/hd`, the release package's two launchers) |
| `config_loader.cpp` | A blank `disc = ""` in game.toml means no disc instead of the game folder | A release install started without the launcher opened its own folder as the disc ("Disc Image Not Found") instead of asking for the player's image |
| `main.cpp` | The text image guard registers the EXE header's `t_size`, not the file length | Release installs read the boot EXE from the player's disc, where it is padded to 512 KB; RAM past the 340 KB of text is game data and overlays |
| `freeze_heartbeat.{c,h}`, `crash_trace.c` | Heartbeat, freeze dumps and the last-run report go to `diagnostics/` next to the exe; no `spin_freeze` while MDEC is decoding | They landed in whatever folder the game was started from, and every boot dumped a false `spin_freeze` during the intro movie |
| `gpu.{c,h}`, `gpu_gl_renderer.h`, `debug_server.c` | Debug only: `vram_upload_ring` / `vram_upload_get` (recent GP0(A0) uploads with pixels), depth24 events in `gl_coh_ring`, load episodes in `turbo_loads` | Diagnosing texture loss and the music release |
