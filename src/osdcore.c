#include "board.h"
#include "osdcore.h"
#include "fonts.h"

osdData_t osdData;

static uint32_t zero = 0;

static void rageMemsetInit(void)
{
    DMA_InitTypeDef DMA_InitStructure;
    DMA_StructInit(&DMA_InitStructure);

    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) & zero;
    //DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Enable;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
    DMA_InitStructure.DMA_BufferSize = sizeof(osdData.OSD_RAM) / 4;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) osdData.OSD_RAM;
    DMA_DeInit(DMA1_Channel1);
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);
}

static __inline void rageMemset(void)
{
    DMA1_Channel1->CCR &= (uint16_t) (~DMA_CCR1_EN);
    DMA1_Channel1->CMAR = (uint32_t) osdData.OSD_RAM;
    DMA1_Channel1->CNDTR = sizeof(osdData.OSD_RAM) / 4;
    DMA1_Channel1->CCR |= DMA_CCR1_EN;
    // DMA1_Channel1->CCR |= DMA_CCR1_EN | DMA_CCR1_TCIE; 
    //DMA_Cmd(DMA1_Channel1, ENABLE);
    //NVIC_EnableIRQ (DMA1_Channel1_IRQn);
}

static __inline int osdPixelOffset(int x, int y)
{
    return (OSD_WIDTH * y + x) >> 3;
}

static __inline void scanlineMod8(int offset, int span, int color)
{
    uint8_t value = color ? 0xFF : 0;
    do {
        osdData.OSD_RAM[offset++] = value;
    } while (--span);
}

static __inline void scanlineMod32(int offset, int span, int color)
{
    uint32_t *pPixels = (uint32_t *) (osdData.OSD_RAM + offset);
    uint32_t value = color ? 0xFFFFFFFF : 0x0;
    offset = 0;

    do {
        pPixels[offset++] = value;
    } while (--span);
}

void osdClearScreen(void)
{
    rageMemset();
}

void osdDrawPixel(int x, int y, int color)
{
    int offset;

    if ((x > OSD_WIDTH - 1) || (y > OSD_HEIGHT - 1) || x < 0 || y < 0)
        return;

    offset = osdPixelOffset(x, y);
    if (color == 1)             // WHITE
        osdData.OSD_RAM[offset] |= 0x80 >> (x & 7);
    else if (color == 0)        // BLACK
        osdData.OSD_RAM[offset] &= ~0x80 >> (x & 7);
    else                        // XOR
        osdData.OSD_RAM[offset] ^= 0x80 >> (x & 7);
}

// more nice drawing Bresenham's line algorithm
void osdDrawLine(int x0, int y0, int x1, int y1, int color)
{
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx, sy;
    int err;
    int e2;

    if (x0 < x1)
        sx = 1;
    else
        sx = -1;

    if (y0 < y1)
        sy = 1;
    else
        sy = -1;

    err = dx - dy;

    while (1) {
        osdDrawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1)
            return;
        e2 = 2 * err;
        if (e2 > -dy) {
            err = err - dy;
            x0 = x0 + sx;
        }
        if (e2 < dx) {
            err = err + dx;
            y0 = y0 + sy;
        }
    }
}

/*
void osdDrawLine(int x1, int y1, int x2, int y2, int color)
{
    unsigned int         i;
    unsigned int         x;
    unsigned int         y;
    int          xinc;
    int          yinc;
    int          dx;
    int          dy;
    int          e;

    // swap x1,y1  with x2,y2
    if (x1 > x2) {
        dx = x1;
        x1 = x2;
        x2 = dx;
        dy = y1;
        y1 = y2;
        y2 = dy;
    }

    dx = x2 - x1;
    dy = y2 - y1;

    x = x1;
    y = y1;

    if (dx < 0) {
        xinc = -1;
        dx = -dx;
    } else {
        xinc = 1;
    }
    if (dy < 0) {
        yinc = -1;
        dy = -dy;
    } else {
        yinc = 1;
    }

    if (dx > dy) {
        e = dy - dx;
        for (i = 0; i <= dx; i++) {
            osdDrawPixel(x, y, color);
            if (e >= 0) {
                e -= dx;
                y += yinc;
            }
            e += dy;
            x += xinc;
        }
    } else {
        e = dx - dy;
        for (i = 0; i <= dy; i++){
            osdDrawPixel(x, y, color);
            if (e >= 0) {
                e -= dy;
                x += xinc;
            }
            e += dx;
            y += yinc;
        }
    }

}
*/

void osdDrawVerticalLine(int x, int y, int height, int color)
{
    int offset = osdPixelOffset(x, y);
    int start = x & 0x7;
    uint8_t mask = 1 << (7 - start);

    if (y + height > OSD_HEIGHT)
        height = OSD_HEIGHT - y;

    while (height--) {
        if (color)
            osdData.OSD_RAM[offset] |= (0xFF & mask);
        else
            osdData.OSD_RAM[offset] &= ~mask;
        offset += OSD_HRES;
    }
}

void osdDrawHorizontalLine(int x, int y, int length, int color)
{
    int offset = osdPixelOffset(x, y);
    int start = x & 0x7;
    int end;

    if (start > 0 || length < 8) {
        uint8_t mask = 0xFF >> start;

        length -= 8 - start;
        if (length < 0) {
            mask &= 0xFF << -length;
            length = 0;
        }
        if (color)
            osdData.OSD_RAM[offset] |= (0xFF & mask);
        else
            osdData.OSD_RAM[offset] &= ~mask;
        offset++;
    }

    end = (length & 0x7);
    length >>= 3;

    if (length > 0) {
        int lenMod32 = length & 0xFFFC;
        if (lenMod32 > 0) {
            scanlineMod32(offset, lenMod32 >> 2, color);
            offset += lenMod32;
            length &= 0x3;
        }
        if (length > 0) {
            scanlineMod8(offset, length, color);
            offset += length;
        }
    }

    if (end > 0) {
        uint8_t mask = 0xFF << (8 - end);
        if (color)
            osdData.OSD_RAM[offset] |= (0xFF & mask);
        else
            osdData.OSD_RAM[offset] &= ~mask;
    }
}

void osdDrawRectangle(int x, int y, int width, int height, int color)
{
    osdDrawHorizontalLine(x, y, width, color);
    osdDrawHorizontalLine(x, y + height - 1, width, color);

    osdDrawVerticalLine(x, y, height, color);
    osdDrawVerticalLine(x + width - 1, y, height, color);
}

void osdDrawCircle(int x, int y, int radius, int color, uint8_t octant_mask)
{
    int offset_x;
    int offset_y;
    int16_t error;

    // Draw only a pixel if radius is zero.
    if (radius == 0) {
        osdDrawPixel(x, y, color);
        return;
    }
    // Set up start iterators.
    offset_x = 0;
    offset_y = radius;
    error = 3 - 2 * radius;

    // Iterate offsetX from 0 to radius.
    while (offset_x <= offset_y) {
        // Draw one pixel for each octant enabled in octant_mask.
        if (octant_mask & GFX_OCTANT0)
            osdDrawPixel(x + offset_y, y - offset_x, color);
        if (octant_mask & GFX_OCTANT1)
            osdDrawPixel(x + offset_x, y - offset_y, color);
        if (octant_mask & GFX_OCTANT2)
            osdDrawPixel(x - offset_x, y - offset_y, color);
        if (octant_mask & GFX_OCTANT3)
            osdDrawPixel(x - offset_y, y - offset_x, color);
        if (octant_mask & GFX_OCTANT4)
            osdDrawPixel(x - offset_y, y + offset_x, color);
        if (octant_mask & GFX_OCTANT5)
            osdDrawPixel(x - offset_x, y + offset_y, color);
        if (octant_mask & GFX_OCTANT6)
            osdDrawPixel(x + offset_x, y + offset_y, color);
        if (octant_mask & GFX_OCTANT7)
            osdDrawPixel(x + offset_y, y + offset_x, color);

        // Update error value and step offset_y when required.
        if (error < 0) {
            error += ((offset_x << 2) + 6);
        } else {
            error += (((offset_x - offset_y) << 2) + 10);
            --offset_y;
        }

        // Next X.
        ++offset_x;
    }
}

void osdDrawFilledCircle(int x, int y, int radius, int color, uint8_t quadrant_mask)
{
    int offset_x;
    int offset_y;
    int16_t error;

    // Draw only a pixel if radius is zero.
    if (radius == 0) {
        osdDrawPixel(x, y, color);
        return;
    }
    // Set up start iterators.
    offset_x = 0;
    offset_y = radius;
    error = 3 - 2 * radius;

    // Iterate offset_x from 0 to radius.
    while (offset_x <= offset_y) {
        // Draw vertical lines tracking each quadrant.
        if (quadrant_mask & GFX_QUADRANT0) {
            osdDrawVerticalLine(x + offset_y, y - offset_x, offset_x + 1, color);
            osdDrawVerticalLine(x + offset_x, y - offset_y, offset_y + 1, color);
        }
        if (quadrant_mask & GFX_QUADRANT1) {
            osdDrawVerticalLine(x - offset_y, y - offset_x, offset_x + 1, color);
            osdDrawVerticalLine(x - offset_x, y - offset_y, offset_y + 1, color);
        }
        if (quadrant_mask & GFX_QUADRANT2) {
            osdDrawVerticalLine(x - offset_y, y, offset_x + 1, color);
            osdDrawVerticalLine(x - offset_x, y, offset_y + 1, color);
        }
        if (quadrant_mask & GFX_QUADRANT3) {
            osdDrawVerticalLine(x + offset_y, y, offset_x + 1, color);
            osdDrawVerticalLine(x + offset_x, y, offset_y + 1, color);
        }
        // Update error value and step offset_y when required.
        if (error < 0) {
            error += ((offset_x << 2) + 6);
        } else {
            error += (((offset_x - offset_y) << 2) + 10);
            --offset_y;
        }

        // Next X.
        ++offset_x;
    }
}

int osd_cursor_X;
int osd_cursor_Y;

void osdSetCursor(int x, int y)
{
    osd_cursor_X = x;
    osd_cursor_Y = y;
}

void osdDrawCharacter(int character, int fontType)
{
    int fontHeight;
    int osdPixelPos;
    int needsAlign;
    int osdRamBytePos;
    int charIndex;
    int stride;
    int word = 0;
    const uint8_t *ptr;
    int i;

    osdPixelPos = osd_cursor_X + (OSD_HRES * 8) * osd_cursor_Y;
    needsAlign = osdPixelPos & 7;
    osdRamBytePos = osdPixelPos >> 3;
    charIndex = (character - 32);
    fontHeight = fonts[fontType].fontHeight;
    stride = (fontHeight * (fonts[fontType].fontWidth >> 3)) + 3;
    ptr = &fonts[fontType].data[charIndex * stride];

    ptr++;                      // skip character index (todo: check availability, if not replace with blank)
    osd_cursor_X += *ptr;       // font width
    ptr += 2;                   // skip font width, height to character data
    if (needsAlign) {
        // 2 byte non-aligned drawing
        for (i = 0; i < fontHeight; i++) {
            if (word) {
                osdData.OSD_RAM[osdRamBytePos] |= *ptr >> needsAlign;
                osdData.OSD_RAM[osdRamBytePos + 1] |= *ptr++ << (8 - needsAlign);
                osdData.OSD_RAM[osdRamBytePos + 1] |= *ptr >> needsAlign;
                osdData.OSD_RAM[osdRamBytePos + 2] |= *ptr++ << (8 - needsAlign);
                osdRamBytePos += OSD_HRES;
            } else {
                osdData.OSD_RAM[osdRamBytePos] |= *ptr >> needsAlign;
                osdData.OSD_RAM[osdRamBytePos + 1] |= *ptr++ << (8 - needsAlign);
                osdRamBytePos += OSD_HRES;
            }
        }
    } else {
        // single byte aligned drawing
        for (i = 0; i < fontHeight; i++) {
            if (word) {
                osdData.OSD_RAM[osdRamBytePos] |= *ptr++;
                osdData.OSD_RAM[osdRamBytePos + 1] |= *ptr++;
                osdRamBytePos += OSD_HRES;
            } else {
                osdData.OSD_RAM[osdRamBytePos] |= *ptr++;
                osdRamBytePos += OSD_HRES;
            }
        }
    }
}

void osdDrawDecimal(int font, int value, int numberLength, int zeroPadded, int decimalPos)
{
    int tmpval;
    int tmpdiv;
    int i, zero = 0;
    const int powers[] = { 0, 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };


    if (value >= 0) {
        osdDrawCharacter(' ', font);
    } else {
        value = -value;
        osdDrawCharacter('-', font);
    }

    tmpval = value;

    for (i = numberLength; i > 0; i--) {
        tmpdiv = tmpval / powers[i];
        tmpval = (value % powers[i]);
        // if (tmpdiv || zeroPadded || decimalPos == i - 1)
        if (tmpdiv || zeroPadded || decimalPos == i - 1 || (tmpdiv == 0 && zero)) {
            osdDrawCharacter(tmpdiv + '0', font);
            zero = 1;
        }
        if (decimalPos == i - 1)
            osdDrawCharacter('.', font);

    }
}

/*
 * Hardware stuff
 */

// #define USESPI2

#ifndef USESPI2
#define OSD_DMA DMA1_Channel3
#define OSD_SPI SPI1
#else
#define OSD_DMA DMA1_Channel5
#define OSD_SPI SPI2
#endif

void osdInit(void)
{
    GPIO_InitTypeDef gpio;
    TIM_TimeBaseInitTypeDef tim;
    NVIC_InitTypeDef nvic;
    DMA_InitTypeDef dma;
    SPI_InitTypeDef spi;

    // turn on peripherals. we're using DMA1, SPI1, TIM1 here
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1 | RCC_APB2Periph_TIM1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

    // dma-based memory clear
    rageMemsetInit();

    // OSD Pins
    GPIO_StructInit(&gpio);
#ifndef USESPI2
    gpio.GPIO_Pin = GPIO_Pin_7; // SPI1_MOSI
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    //gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &gpio);
#else
    gpio.GPIO_Pin = GPIO_Pin_15;        // SPI2_MOSI
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);
#endif


    gpio.GPIO_Pin = GPIO_Pin_15;        // OSD_BW
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    //gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &gpio);
    digitalHi(GPIOB, GPIO_Pin_15);
    //digitalLo(GPIOB, GPIO_Pin_15);

    gpio.GPIO_Pin = GPIO_Pin_8; // TIM1_CH1 HSYNC
    gpio.GPIO_Mode = GPIO_Mode_IPD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    //gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_8; // TIM4 CH3 VSYNC
    gpio.GPIO_Mode = GPIO_Mode_IPD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);

    // OSD SPI
    SPI_StructInit(&spi);
    spi.SPI_Direction = SPI_Direction_1Line_Tx;
    spi.SPI_Mode = SPI_Mode_Master;
    spi.SPI_DataSize = SPI_DataSize_8b;
    // spi.SPI_DataSize = SPI_DataSize_16b;
    spi.SPI_CPOL = SPI_CPOL_Low;
    spi.SPI_CPHA = SPI_CPHA_1Edge;
    spi.SPI_NSS = SPI_NSS_Soft;
    // spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2; 
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;        // 400 pixels on x axis
    //spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4; // ~800 pixels on x axis
    //spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16; // ~200 pixels on x axis
    spi.SPI_FirstBit = 0;
    spi.SPI_CRCPolynomial = 0;
    SPI_Init(OSD_SPI, &spi);
    SPI_Cmd(OSD_SPI, ENABLE);

    // OSD DMA DMA1_Channel3
    DMA_StructInit(&dma);
    DMA_DeInit(OSD_DMA);
    dma.DMA_PeripheralBaseAddr = (uint32_t) & OSD_SPI->DR;
    dma.DMA_MemoryBaseAddr = (uint32_t) osdData.OSD_RAM;
    dma.DMA_DIR = DMA_DIR_PeripheralDST;
    dma.DMA_BufferSize = OSD_HRES;
    dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dma.DMA_Mode = DMA_Mode_Normal;
    dma.DMA_Priority = DMA_Priority_VeryHigh;
    //dma.DMA_Priority = DMA_Priority_High;
    dma.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(OSD_DMA, &dma);

    // OSD DMA for SPI setup
    DMA_Cmd(OSD_DMA, ENABLE);
    SPI_I2S_DMACmd(OSD_SPI, SPI_I2S_DMAReq_Tx, ENABLE);

    // OSD Timer (TIM1@72MHz)
    TIM_TimeBaseStructInit(&tim);
    tim.TIM_Period = 0xFFFF;
    tim.TIM_Prescaler = 0;
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM1, &tim);

    // reset timer on each edge (HSYNC)
    TIM_SelectInputTrigger(TIM1, TIM_TS_TI1FP1);
    TIM_ETRConfig(TIM1, TIM_ExtTRGPSC_OFF, TIM_ExtTRGPolarity_NonInverted, 50);
    TIM_SelectSlaveMode(TIM1, TIM_SlaveMode_Reset);
    TIM_ITConfig(TIM1, TIM_IT_CC1, ENABLE);

    // set compare value
    //TIM_SetCompare1(TIM1, 460); // shift left/right osd screen
    //TIM_SetCompare1(TIM1, 430);

    TIM_SetCompare1(TIM1, 480); // timing depended from irq handler


    nvic.NVIC_IRQChannel = TIM1_CC_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 0;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
    TIM_Cmd(TIM1, ENABLE);

    osdData.osdUpdateFlag = CoCreateFlag(0, 0);
}





// HSYNC
// PAL/SECAM have 625/50Hz and 288 active, NTSC 525 and 243 (242) active
void TIM1_CC_IRQHandler(void)
{
    static int PAL = 0;         // default if NTSC, can switched to PAL, but not contrariwise
    int slpos, slmax;
    static int maxline = 0;
    static int inv = 0;
    int line;


    CoEnterISR();

    slpos = PAL ? 45 : 24;      // used for shift up/down area screen
    slmax = slpos + OSD_HEIGHT;
    osdData.PAL = PAL;

    if (TIM1->SR & TIM_SR_CC1IF) {      // capture interrupt
        if (!(GPIOB->IDR & GPIO_Pin_8) && (osdData.currentScanLine > 100)) {    // wait VSYNC
            // Note: got max 309-314 for PAL and must be 264 and more in NTSC
            if (maxline < osdData.currentScanLine)
                maxline = osdData.currentScanLine;

            //if (maxline) {
            osdData.maxScanLine = maxline;
            PAL = maxline > 300 ? 1 : 0;        // recheck mode
            //}
            osdData.currentScanLine = 0;
            maxline = 0;
        } else if (GPIOA->IDR & GPIO_Pin_8) {   // check HSYNC
            osdData.currentScanLine++;
            //}

            // lets out video line
            if (osdData.currentScanLine >= slpos && osdData.currentScanLine <= slmax - 1) {
                line = osdData.currentScanLine - slpos;

                /*
                   // make interlaced out odd/even frames
                   if (inv) {
                   digitalHi(GPIOB, GPIO_Pin_15);
                   } else {
                   digitalLo(GPIOB, GPIO_Pin_15);
                   } 
                 */

                osdData.OSD_RAM[OSD_HRES * line + (OSD_HRES - 1)] = 0;
                OSD_DMA->CCR &= (uint16_t) (~DMA_CCR1_EN);
                OSD_DMA->CMAR = (uint32_t) & osdData.OSD_RAM[OSD_HRES * line];
                OSD_DMA->CNDTR = OSD_HRES;
                OSD_DMA->CCR |= DMA_CCR1_EN;    // run output
            }
/*				
						// clear line
				if (!inv && osdData.currentScanLine >= slpos - 1 && osdData.currentScanLine <= slmax - 2) {
					  line = osdData.currentScanLine - slpos - 1;
						DMA1_Channel1->CCR &= (uint16_t)(~DMA_CCR1_EN);
						DMA1_Channel1->CMAR = (uint32_t)&osdData.OSD_RAM[OSD_HRES * line]; 
						DMA1_Channel1->CNDTR = OSD_HRES / 4 + 1;
						DMA1_Channel1->CCR |= DMA_CCR1_EN; 
				}	
*/

            /*
               if (!inv && osdData.currentScanLine == slmax - 10) {
               osdClearScreen();
               } */

            // we have last line in frame?
            if (osdData.currentScanLine == slmax) {     // max line
                inv ^= 1;
                /*                if (inv2++ == 25) {
                   inv2 = 0;
                   if (osdData.maxScanLine > 300) 
                   PAL = 1; 
                   else 
                   PAL = 0;   // cant be, coz if cur mode is PAL with NTSC in signal, we dont reach it
                   osdData.maxScanLine = 0;  // recalc max sometimes
                   } */
                if (inv) {      // no need to redraw each frame
                    osdClearScreen();
                    isr_SetFlag(osdData.osdUpdateFlag);
                } else {
                    isr_SetFlag(osdData.osdRecalcFlag);
                }
            }
        }
        TIM_ClearFlag(TIM1, TIM_FLAG_CC1);
    }

    CoExitISR();
}
