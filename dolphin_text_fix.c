/*
 * Dolphin Dark Text Fix (LD_PRELOAD Shim)
 *
 * Fixes unreadable / pitch-black text on dark themes in KDE Dolphin
 * when running outside KDE Plasma (e.g., on Niri, Sway, Hyprland, i3).
 *
 * Root Cause:
 * In Dolphin, file and folder items are rendered via QGraphicsScene in KItemListView.
 * While Dolphin sets the viewport background from kdeglobals, KItemListView
 * inherits an uninitialized default QPalette (black text: #000000).
 * Since QGraphicsWidget is not a standard QWidget, it never receives
 * QEvent::PaletteChange when Qt themes are switched.
 *
 * This shim intercepts textColor() in KStandardItemListWidget and KItemListGroupHeader,
 * returning a readable light text color while preserving selection text colors.
 */

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    uint64_t r1;
    uint64_t r2;
} TwoU64;

static uint8_t g_main_r = 242;
static uint8_t g_main_g = 242;
static uint8_t g_main_b = 243;

static uint8_t g_sub_r = 175;
static uint8_t g_sub_g = 179;
static uint8_t g_sub_b = 182;

static int g_inited = 0;

static void parse_hex_color(const char *hex, uint8_t *r, uint8_t *g, uint8_t *b) {
    if (!hex) return;
    if (hex[0] == '#') hex++;
    unsigned int val = 0;
    if (sscanf(hex, "%x", &val) == 1) {
        *r = (val >> 16) & 0xff;
        *g = (val >> 8) & 0xff;
        *b = val & 0xff;
    }
}

static void init_colors(void) {
    if (g_inited) return;
    g_inited = 1;
    const char *main_env = getenv("DOLPHIN_TEXT_COLOR");
    if (main_env) {
        parse_hex_color(main_env, &g_main_r, &g_main_g, &g_main_b);
    }
    const char *sub_env = getenv("DOLPHIN_SUBTEXT_COLOR");
    if (sub_env) {
        parse_hex_color(sub_env, &g_sub_r, &g_sub_g, &g_sub_b);
    }
}

static void set_qcolor(uint16_t *p, uint8_t r, uint8_t g, uint8_t b) {
    // Qt 6 QColor layout (16 bytes):
    // p[0..1]: Spec = Rgb (uint32_t 1)
    // p[2]:    Alpha (uint16_t 0xffff = 65535 / 255)
    // p[3]:    Red   (uint16_t: val | (val << 8))
    // p[4]:    Green (uint16_t: val | (val << 8))
    // p[5]:    Blue  (uint16_t: val | (val << 8))
    // p[6..7]: padding / float representation
    p[0] = 1;
    p[1] = 0;
    p[2] = 0xffff;
    p[3] = (uint16_t)(r | (r << 8));
    p[4] = (uint16_t)(g | (g << 8));
    p[5] = (uint16_t)(b | (b << 8));
    p[6] = 0;
    p[7] = 0;
}

// 1. Main filename/foldername text in Icon, Compact, and Details views
TwoU64 _ZNK23KStandardItemListWidget9textColorERK7QWidget(void *this, void *w) {
    TwoU64 (*real)(void*, void*) = dlsym(RTLD_NEXT, "_ZNK23KStandardItemListWidget9textColorERK7QWidget");
    TwoU64 res = real(this, w);

    int (*isSelected)(void*) = dlsym(RTLD_NEXT, "_ZNK15KItemListWidget10isSelectedEv");
    if (isSelected && isSelected(this)) {
        return res; // Keep selection highlight text color intact
    }

    init_colors();
    set_qcolor((uint16_t*)&res, g_main_r, g_main_g, g_main_b);
    return res;
}

// 2. Secondary text (file size, item count, date, details)
void _ZN23KStandardItemListWidget31updateAdditionalInfoTextColorEv(void *this) {
    void (*real)(void*) = dlsym(RTLD_NEXT, "_ZN23KStandardItemListWidget31updateAdditionalInfoTextColorEv");
    real(this);

    init_colors();
    // Offset 0x248 holds m_additionalInfoTextColor in KStandardItemListWidget
    uint16_t *p = (uint16_t*)((char*)this + 0x248);
    set_qcolor(p, g_sub_r, g_sub_g, g_sub_b);
}

// 3. Category / Group headers (e.g., grouped by date, type, etc.)
TwoU64 _ZNK20KItemListGroupHeader9textColorEv(void *this) {
    TwoU64 (*real)(void*) = dlsym(RTLD_NEXT, "_ZNK20KItemListGroupHeader9textColorEv");
    TwoU64 res = real(this);

    init_colors();
    set_qcolor((uint16_t*)&res, g_main_r, g_main_g, g_main_b);
    return res;
}
