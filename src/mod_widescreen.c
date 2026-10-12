/*
 * Azure Dreams: experimental 16:9 widescreen.
 *
 * Selected by the "azuredreams.widescreen" package (default off). The runtime
 * owns widescreen on PSX and only engages it when a trusted plugin asks for a
 * wide display aspect before the window is created. Native-wide rendering then
 * widens the view without squashing, and the [widescreen.cull] sites in
 * game.toml widen the game's screen-edge rejects by the live reveal margin
 * (zero at 4:3, so the disabled build is unchanged).
 *
 * Terrain: the town/dungeon renderer does not cull tiles against the screen.
 * func_80046884 takes a screen-space view rectangle (camera + 24: x, y, w, h,
 * relative to the screen centre), casts its four corners onto the ground
 * plane, and func_80046C20 scan-converts that footprint into the set of tiles
 * and walls to draw. The overlays initialize the rectangle once per area to
 * x = -188, w = 370 (320 px plus a margin), so the 16:9 side strips fell
 * outside the footprint and tiles popped in along diagonal edges. While
 * widescreen is engaged, widen that rectangle by the reveal margin on both
 * sides. Only the stock values are touched, so any other view mode the game
 * sets up is left alone.
 */
#include "mod_plugins.h"

#define CAMERA_VIEW_RECT_X 0x80083178u /* camera (0x80083160) + 24 */
#define CAMERA_VIEW_RECT_W 0x8008317Cu /* camera + 28 */
#define STOCK_VIEW_RECT_X  (-188)
#define STOCK_VIEW_RECT_W  370

static void azure_widescreen_activate(void) {
    (void)psx_mod_set_fixed_display_aspect(16u, 9u);
}

static void azure_widescreen_vblank(void) {
    if (!psx_mod_game_started()) return;
    const int32_t margin = psx_mod_widescreen_x_margin();
    if (margin <= 0) return;
    const int16_t x = (int16_t)psx_mod_read_half(CAMERA_VIEW_RECT_X);
    const int16_t w = (int16_t)psx_mod_read_half(CAMERA_VIEW_RECT_W);
    if (x != STOCK_VIEW_RECT_X || w != STOCK_VIEW_RECT_W) return;
    psx_mod_write_half(CAMERA_VIEW_RECT_X, (uint16_t)(int16_t)(STOCK_VIEW_RECT_X - margin));
    psx_mod_write_half(CAMERA_VIEW_RECT_W, (uint16_t)(int16_t)(STOCK_VIEW_RECT_W + 2 * margin));
}

PSX_MOD_CONSTRUCTOR(azure_register_widescreen_plugin) {
    (void)psx_mod_register_activation_plugin("azuredreams.widescreen",
                                             azure_widescreen_activate);
    (void)psx_mod_register_vblank_plugin("azuredreams.widescreen",
                                         azure_widescreen_vblank);
}
