#pragma once

 #define OSD_WIDTH   368
#define OSD_HEIGHT  240  // NTSC max
//#define OSD_HEIGHT  (270)     // PAL max

#define OSD_HRES    (OSD_WIDTH / 8)
#define OSD_VRES OSD_HEIGHT

typedef struct {
    volatile uint8_t OSD_RAM[OSD_HRES * OSD_VRES];
    volatile uint8_t OSD_LINE[OSD_HRES + 1];
    volatile uint8_t OSD_LINEBW[OSD_HRES + 1];
    OS_FlagID osdUpdateFlag;
    OS_FlagID osdRecalcFlag;
    uint16_t currentScanLine;
    uint16_t maxScanLine;
    uint8_t PAL;                    // PAL or NTSC
    uint8_t *ptr;                   // current pos in OSD_RAM for DMA irq refresh
} osdData_t;

//! Bitmask for drawing circle octant 0.
#define GFX_OCTANT0     (1 << 0)
//! Bitmask for drawing circle octant 1.
#define GFX_OCTANT1     (1 << 1)
//! Bitmask for drawing circle octant 2.
#define GFX_OCTANT2     (1 << 2)
//! Bitmask for drawing circle octant 3.
#define GFX_OCTANT3     (1 << 3)
//! Bitmask for drawing circle octant 4.
#define GFX_OCTANT4     (1 << 4)
//! Bitmask for drawing circle octant 5.
#define GFX_OCTANT5     (1 << 5)
//! Bitmask for drawing circle octant 6.
#define GFX_OCTANT6     (1 << 6)
//! Bitmask for drawing circle octant 7.
#define GFX_OCTANT7     (1 << 7)
//! Bitmask for drawing circle quadrant 0.
#define GFX_QUADRANT0   (GFX_OCTANT0 | GFX_OCTANT1)
//! Bitmask for drawing circle quadrant 1.
#define GFX_QUADRANT1   (GFX_OCTANT2 | GFX_OCTANT3)
//! Bitmask for drawing circle quadrant 2.
#define GFX_QUADRANT2   (GFX_OCTANT4 | GFX_OCTANT5)
//! Bitmask for drawing circle quadrant 3.
#define GFX_QUADRANT3   (GFX_OCTANT6 | GFX_OCTANT7)


void osdInit(void);
void osdClearScreen(void);
void osdTestPattern(void);
void osdDrawPixel(int x, int y, int color);
void osdDrawLine(int x1, int y1, int x2, int y2, int color, int type);
void osdDrawHorizontalLine(int x, int y, int length, int color);
void osdDrawVerticalLine(int x, int y, int height, int color);
void osdDrawRectangle(int x, int y, int width, int height, int color);
void osdDrawCircle(int x, int y, int radius, int color, uint8_t octant_mask);
void osdDrawFilledCircle(int x, int y, int radius, int color, uint8_t quadrant_mask);
void osdSetCursor(int x, int y);
void osdDrawCharacter(int character, int fontType);
void osdDrawDecimal(int font, int value, int numberLength, int zeroPadded, int decimalPos);
void osdDrawDecimal2(int font, int value, int numberLength, int zeroPadded, int decimalPos);

extern osdData_t osdData;
