#pragma once

typedef struct {
    uint8_t fontHeight;
    uint8_t fontWidth;
    const uint8_t *data;
} fontdata_t;

extern fontdata_t fonts[];

// sync with font list at bottom of fonts.c
enum {
    /* 0 */ FONT_TINY_NUMBERS,
    /* 1 */ FONT_8PX_PROP,
    /* 2 */ FONT_8PX_PROP_BOLD,
    /* 3 */ FONT_16PX_WIDE_FIXED,
    /* 4 */ FONT_16PX_FIXED,
    /* 5 */ FONT_8PX_FIXED,
    FONT_LAST
};
