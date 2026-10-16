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
#include "cpu_state.h"

#define CAMERA_VIEW_RECT_X 0x80083178u /* camera (0x80083160) + 24 */
#define CAMERA_VIEW_RECT_W 0x8008317Cu /* camera + 28 */
#define STOCK_VIEW_RECT_X  (-188)
#define STOCK_VIEW_RECT_W  370

/* Player HUD panel (LV / HP, top-left). Its sprite groups are drawn through
 * func_8004C36C with an identity "screen" camera, positioned by a table of
 * (x, y) anchors relative to the screen centre. In 16:9 the panel stayed at
 * the 4:3 left edge, inset by the reveal; entry 0 is a gauge parked just
 * off-screen that the widened view exposed. Shift every entry left by the
 * margin so the panel hugs the wide edge and the gauge is hidden again. */
#define HUD_LEFT_ANCHORS   0x800E2924u /* 4 x {int16 x, int16 y} */
static const int16_t k_hud_left_stock_x[4] = { -196, -140, -90, -34 };
static const int16_t k_hud_left_stock_y[4] = { -138, -86, -138, -82 };

/* Dungeon minimap (translucent map, right of centre). func_8009E798 (dungeon
 * overlay, reached through a function pointer) draws it; its second argument
 * points to a heap-allocated state struct whose +2 halfword is the map's
 * horizontal screen anchor (stock 256; +6 is the vertical, 120). Raising the
 * anchor by the margin moves the map to the wide right edge. The struct moves
 * between floors, so it is taken from $a1 at function entry via
 * [recompiler] mod_function_entry_funcs. Overlays reuse addresses, so the
 * hook first checks it is really this function (prologue words) and that the
 * struct holds the stock layout. */
#define MINIMAP_FUNC        0x8009E798u
#define MINIMAP_FUNC_WORD0  0x27BDFFB8u /* addiu sp, sp, -72 */
#define MINIMAP_FUNC_WORD2  0x00A0B021u /* addu  s6, a1, zero */
#define MINIMAP_STOCK_X     256
#define MINIMAP_STOCK_Y     120
/* The map is clipped to a window RECT in the first argument's struct: $a0+4 =
 * {x 213, y 56, w 107, h 112} (right edge = 319, the 4:3 edge), with a copy of
 * {x, y} at +12. The window must move with the anchor; the native-wide
 * renderer (local psxrecomp patch) mirrors such framebuffer clip windows into
 * the reveal strip and keeps the canonical draw inside the framebuffer. */
#define MINIMAP_WIN_STOCK_X 213
#define MINIMAP_WIN_STOCK_Y 56
#define MINIMAP_WIN_STOCK_W 107
#define MINIMAP_WIN_STOCK_H 112
/* Stock, the window reaches 64 px right of and above the map anchor (256, 120)
 * but only 43 px left and 48 px below, so larger maps were cut off on the left
 * and bottom. With the map on the wide edge the window is grown to 64 px on
 * every side: x 192, w 128, h 128 (the right edge stays on the screen edge). */
#define MINIMAP_WIN_WIDE_X  (MINIMAP_STOCK_X - 64)
#define MINIMAP_WIN_WIDE_W  128
#define MINIMAP_WIN_WIDE_H  128

static void azure_widescreen_activate(void) {
    (void)psx_mod_set_fixed_display_aspect(16u, 9u);
}

static void widen_view_rect(int32_t margin) {
    const int16_t x = (int16_t)psx_mod_read_half(CAMERA_VIEW_RECT_X);
    const int16_t w = (int16_t)psx_mod_read_half(CAMERA_VIEW_RECT_W);
    if (x != STOCK_VIEW_RECT_X || w != STOCK_VIEW_RECT_W) return;
    psx_mod_write_half(CAMERA_VIEW_RECT_X, (uint16_t)(int16_t)(STOCK_VIEW_RECT_X - margin));
    psx_mod_write_half(CAMERA_VIEW_RECT_W, (uint16_t)(int16_t)(STOCK_VIEW_RECT_W + 2 * margin));
}

static void anchor_left_hud(int32_t margin) {
    /* Only an exact stock table is shifted: once shifted it no longer matches,
     * and anything else living at this overlay address is left alone. */
    for (int i = 0; i < 4; i++) {
        const uint32_t a = HUD_LEFT_ANCHORS + 4u * (uint32_t)i;
        if ((int16_t)psx_mod_read_half(a) != k_hud_left_stock_x[i] ||
            (int16_t)psx_mod_read_half(a + 2u) != k_hud_left_stock_y[i])
            return;
    }
    for (int i = 0; i < 4; i++)
        psx_mod_write_half(HUD_LEFT_ANCHORS + 4u * (uint32_t)i,
                           (uint16_t)(int16_t)(k_hud_left_stock_x[i] - margin));
}

static void azure_widescreen_vblank(void) {
    if (!psx_mod_game_started()) return;
    const int32_t margin = psx_mod_widescreen_x_margin();
    if (margin <= 0) return;
    widen_view_rect(margin);
    anchor_left_hud(margin);
}

static void azure_widescreen_minimap_entry(struct CPUState *cpu, uint32_t address) {
    (void)address;
    const int32_t margin = psx_mod_widescreen_x_margin();
    if (margin <= 0) return;
    if (psx_mod_read_word(MINIMAP_FUNC) != MINIMAP_FUNC_WORD0 ||
        psx_mod_read_word(MINIMAP_FUNC + 8u) != MINIMAP_FUNC_WORD2)
        return;
    const uint32_t win   = cpu->gpr[4];                 /* $a0: window struct */
    const uint32_t state = cpu->gpr[5];                 /* $a1: map state */
    if ((win & 0xFFE00000u) != 0x80000000u ||
        (state & 0xFFE00000u) != 0x80000000u) return;   /* main RAM only */

    /* Window: move and grow only an exact stock RECT (and shift its x copy at
     * +12 as before). */
    if ((int16_t)psx_mod_read_half(win + 4u)  == MINIMAP_WIN_STOCK_X &&
        (int16_t)psx_mod_read_half(win + 6u)  == MINIMAP_WIN_STOCK_Y &&
        (int16_t)psx_mod_read_half(win + 8u)  == MINIMAP_WIN_STOCK_W &&
        (int16_t)psx_mod_read_half(win + 10u) == MINIMAP_WIN_STOCK_H) {
        psx_mod_write_half(win + 4u, (uint16_t)(int16_t)(MINIMAP_WIN_WIDE_X + margin));
        psx_mod_write_half(win + 8u, (uint16_t)MINIMAP_WIN_WIDE_W);
        psx_mod_write_half(win + 10u, (uint16_t)MINIMAP_WIN_WIDE_H);
        if ((int16_t)psx_mod_read_half(win + 12u) == MINIMAP_WIN_STOCK_X)
            psx_mod_write_half(win + 12u, (uint16_t)(int16_t)(MINIMAP_WIN_STOCK_X + margin));
    }
    /* Map content anchor. */
    if ((int16_t)psx_mod_read_half(state + 2u) == MINIMAP_STOCK_X &&
        (int16_t)psx_mod_read_half(state + 6u) == MINIMAP_STOCK_Y)
        psx_mod_write_half(state + 2u, (uint16_t)(int16_t)(MINIMAP_STOCK_X + margin));
}

PSX_MOD_CONSTRUCTOR(azure_register_widescreen_plugin) {
    (void)psx_mod_register_activation_plugin("azuredreams.widescreen",
                                             azure_widescreen_activate);
    (void)psx_mod_register_vblank_plugin("azuredreams.widescreen",
                                         azure_widescreen_vblank);
    (void)psx_mod_register_function_entry_plugin("azuredreams.widescreen",
                                                 MINIMAP_FUNC,
                                                 azure_widescreen_minimap_entry);
}
