#include "board.h"
#include "osdcore.h"
#include "fonts.h"

osdData_t osdData;


// 5  - 14.39885 ~758
// 6  - 11.99904 ~631
// 7  - 10.28489 ~541
// 8  - 8.99928  ~473
// 9  - 7.99936  ~421
// 10 - 7.199424 ~378
// 11 - 6.54931  ~344
// 12 - 5.99952  ~315

//PAL
//Line time is 64 microseconds
//Line count is 625

//NTSC
//Line time is 63.49
//

static uint32_t zero = 0;

//Setup a dma channel to overwrite the entire osd with 0's
static void rageMemsetInit(void)
{
    DMA_InitTypeDef DMA_InitStructure;
    DMA_StructInit(&DMA_InitStructure);

    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) & zero;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    //DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Enable;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    //DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    //DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_BufferSize = sizeof(osdData.OSD_RAM) / 4;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) osdData.OSD_RAM;
    DMA_DeInit(DMA1_Channel1);
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);
}

//Start the osd clearing dma transfer
static __inline void rageMemset(void)
{
    DMA1_Channel1->CCR &= (uint16_t) (~DMA_CCR1_EN);
    DMA1_Channel1->CMAR = (uint32_t) osdData.OSD_RAM;
    //DMA1_Channel1->CNDTR = sizeof(osdData.OSD_RAM) / 4;
    DMA1_Channel1->CNDTR = sizeof(osdData.OSD_RAM);
    DMA1_Channel1->CCR |= DMA_CCR1_EN;
    // DMA1_Channel1->CCR |= DMA_CCR1_EN | DMA_CCR1_TCIE; 
    //DMA_Cmd(DMA1_Channel1, ENABLE);
    //NVIC_EnableIRQ (DMA1_Channel1_IRQn);
}

static __inline int osdPixelOffset(int x, int y)
{
    return (OSD_HRES * y) + (x >> 3);
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

void osdDrawPixel(uint32_t x, uint32_t y, int color)
{
    int offset;

    //Clip out of range
    if ((x >= OSD_WIDTH_MAX ) || (y >= OSD_HEIGHT_MAX) )
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
void osdDrawLine(int x0, int y0, int x1, int y1, int color, int type)
{
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx, sy;
    int err;
    int e2;
    int skip = 0;

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
        if (!skip)
            osdDrawPixel(x0, y0, color);
        if (skip-- <= 0)
            skip = type;
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

void osdDrawVerticalLine(int x, int y, int height, int color)
{
    int offset = osdPixelOffset(x, y);
    int start = x & 0x7;
    uint8_t mask = 1 << (7 - start);

#if 0
    if ( y >= OSD_HEIGHT_MAX )
    	return;
    if (y + height > OSD_HEIGHT_MAX)
        height = OSD_HEIGHT_MAX - y;
#endif

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
    
    if ((osd_cursor_X > OSD_WIDTH_MAX - 1 - *ptr) || (osd_cursor_Y > OSD_HEIGHT_MAX - 1 - fontHeight) || osd_cursor_X < 0 || osd_cursor_Y < 0)
        return;

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

void osdDrawDecimal2(int font, int value, int numberLength, int zeroPadded, int decimalPos)
{
    int tmpval;
    int tmpdiv;
    int i, zero = 0;
    const int powers[] = { 0, 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };

    tmpval = value;

    for (i = numberLength; i > 0; i--) {
        tmpdiv = tmpval / powers[i];
        tmpval = (value % powers[i]);
        if (tmpdiv || zeroPadded || decimalPos == i - 1 || (tmpdiv == 0 && zero)) {
            osdDrawCharacter(tmpdiv + '0', font);
            zero = 1;
        }
        if (decimalPos == i - 1)
            osdDrawCharacter('.', font);

    }
}


void osdDrawString( int x, int y, int font, const char* line ) {
	osdSetCursor( x, y );

	for( ; *line; line++ ) {
		osdDrawCharacter( *line, font );
	}
}


static void osdFormatPut( void* data, char ch ) {
	int* font = (int*)data;
	osdDrawCharacter( ch, *font );
}

void osdDrawFormat( int x, int y, int font, const char* fmt, ... ) {
	osdSetCursor( x, y );

	va_list va;
    va_start(va, fmt);
    formatOutputVA( osdFormatPut, &font, fmt, va );
    va_end(va);
}

/*
 * Hardware stuff
 */

#define OSD_DMA DMA1_Channel3
#define OSD_SPI SPI1
#define OSDBW_DMA DMA1_Channel5
#define OSDBW_SPI SPI2

void osdInit(void)
{
    GPIO_InitTypeDef gpio;
    TIM_TimeBaseInitTypeDef tim;
    NVIC_InitTypeDef nvic;
    DMA_InitTypeDef dma;
    SPI_InitTypeDef spi;
    TIM_OCInitTypeDef timoc;

    // turn on peripherals. we're using SPI1, SPI2, TIM1 and TIM3 here
    // DMA1 has already been enabled from main
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1 | RCC_APB2Periph_TIM1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2 | RCC_APB1Periph_TIM3, ENABLE);

    // dma-based memory clear
    rageMemsetInit();

    // OSD SPI1 Pins
    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = GPIO_Pin_6; // SPI1 MISO
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7; // SPI1 MOSI + CLK slave
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    // OSD SPI
    SPI_StructInit(&spi);
    spi.SPI_Direction = SPI_Direction_1Line_Tx;
    spi.SPI_Mode = SPI_Mode_Slave;
    spi.SPI_DataSize = SPI_DataSize_8b;
    spi.SPI_CPOL = SPI_CPOL_Low;
    spi.SPI_CPHA = SPI_CPHA_1Edge;
    spi.SPI_NSS = SPI_NSS_Soft;
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
    NVIC_EnableIRQ(DMA1_Channel3_IRQn);      // enable

    // OSD DMA for SPI setup
    DMA_Cmd(OSD_DMA, ENABLE);
    SPI_I2S_DMACmd(OSD_SPI, SPI_I2S_DMAReq_Tx, ENABLE);

    // BW SPI2 Pins
    gpio.GPIO_Pin = GPIO_Pin_14; // SPI2_MISO
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_15; // SPI2_SCK SPI2_MOSI
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &gpio);

    // OSD SPI2 BW
    SPI_StructInit(&spi);
    spi.SPI_Direction = SPI_Direction_1Line_Tx;
    spi.SPI_Mode = SPI_Mode_Slave;
    spi.SPI_DataSize = SPI_DataSize_8b;
    spi.SPI_CPOL = SPI_CPOL_Low;
    spi.SPI_CPHA = SPI_CPHA_1Edge;
    spi.SPI_NSS = SPI_NSS_Soft;
    spi.SPI_FirstBit = 0;
    spi.SPI_CRCPolynomial = 0;
    SPI_Init(OSDBW_SPI, &spi);
    SPI_Cmd(OSDBW_SPI, ENABLE);

    // OSDBW DMA DMA1_Channel5
    DMA_StructInit(&dma);
    DMA_DeInit(OSDBW_DMA);
    dma.DMA_PeripheralBaseAddr = (uint32_t) &OSDBW_SPI->DR;
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
    DMA_Init(OSDBW_DMA, &dma);

    // OSDBW DMA for SPI
    DMA_Cmd(OSDBW_DMA, ENABLE);
    SPI_I2S_DMACmd(OSDBW_SPI, SPI_I2S_DMAReq_Tx, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_8; // TIM1_CH1 HSYNC
    gpio.GPIO_Mode = GPIO_Mode_IPD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    //Nothing triggered on here just for reading out
    gpio.GPIO_Pin = GPIO_Pin_8; // TIM4 CH3 VSYNC
    gpio.GPIO_Mode = GPIO_Mode_IPD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);

    // OSD Timer (TIM1@72MHz)
    //This starts counting when hsync starts
    TIM_TimeBaseStructInit(&tim);
    tim.TIM_Period = 0xFFFF;
    tim.TIM_Period = 2 * cfg.clockDivider * cfg.delayX;
    tim.TIM_Prescaler = 0;
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM1, &tim);

    // reset timer on each edge (HSYNC)
    TIM_SelectInputTrigger(TIM1, TIM_TS_TI1FP1 );
    //TIM_ETRConfig(TIM1, TIM_ExtTRGPSC_OFF, TIM_ExtTRGPolarity_NonInverted, 50 );
    TIM_ETRConfig(TIM1, TIM_ExtTRGPSC_OFF, TIM_ExtTRGPolarity_NonInverted, 0 );
    //Whenever my trigger goes, reset the timer and start it
//    TIM_SelectSlaveMode(TIM1, TIM_SlaveMode_Reset );
    TIM_SelectSlaveMode(TIM1, TIM_SlaveMode_Trigger );
    ///Do  single pulse when this update it triggers tim3
    TIM_SelectOnePulseMode( TIM1, TIM_OPMode_Single );
    //Do a single counter and wait till next trigger
    TIM_SetCompare1(TIM1, cfg.clockDivider * cfg.delayX );

    //Setup output 1 to be the master trigger for my slaves
//    TIM_SelectOutputTrigger( TIM1, TIM_TRGOSource_Update );
    TIM_SelectOutputTrigger( TIM1, TIM_TRGOSource_OC1 );

    //Generate an irq when you hit compare 1
//    TIM_ITConfig(TIM1, TIM_IT_CC1, ENABLE);
//    TIM_ITConfig(TIM1, TIM_IT_CC2, ENABLE);
//    TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
    //Generate an irq when TIM1 gets triggered by hsync
    TIM_ITConfig(TIM1, TIM_IT_Trigger, ENABLE);
    
    //Enable IRQ for TIM1 trigger raise
    //Give it the highest priority level
    nvic.NVIC_IRQChannel = TIM1_TRG_COM_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 0;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    // SPI timing GPIO out (TIM3_CH3)
    gpio.GPIO_Pin = GPIO_Pin_0; // TIM3_CH3
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);

    // TIM3C3 clk for SPI1/SPI2 in slave
    TIM_TimeBaseStructInit(&tim);
    tim.TIM_Prescaler = 0;
    tim.TIM_Period = cfg.clockDivider;
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &tim);
    
    //internal trigger for TIM3 should be TIM1
    TIM_SelectInputTrigger( TIM3, TIM_TS_ITR0 );
    //When tim1 sends it's trigger tim3 starts
    TIM_SelectSlaveMode( TIM3, TIM_SlaveMode_Trigger );
    //Better master/slave syncing?
    TIM_SelectMasterSlaveMode( TIM3, TIM_MasterSlaveMode_Enable );

    //Generate a pwm style on/off ouput on TIM3
    TIM_OCStructInit( &timoc );
    timoc.TIM_OutputState = TIM_OutputState_Enable;
    timoc.TIM_OutputNState = TIM_OutputNState_Disable;
    timoc.TIM_OCMode = TIM_OCMode_PWM2;
    timoc.TIM_Pulse = 2;
    timoc.TIM_OCPolarity = TIM_OCPolarity_Low;
    timoc.TIM_OCIdleState = TIM_OCIdleState_Set;
    TIM_OC3Init(TIM3, &timoc );

    TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);

    memset((void *)osdData.OSD_LINEBW, 0xFF, sizeof(osdData.OSD_LINEBW));  // white only
    osdData.PAL = 0;  // default is NTSC
    osdData.osdUpdateFlag = CoCreateFlag(0, 0);
}

void osdRestart() {
    TIM_SetAutoreload( TIM1, 2 * cfg.clockDivider * cfg.delayX );
    TIM_SetCompare1(TIM1, cfg.clockDivider * cfg.delayX );
    TIM_SetAutoreload( TIM3, cfg.clockDivider );
}

// DMA_OSD (end of pixels line)
void DMA1_Channel3_IRQHandler(void) 
{
    if(DMA1->ISR & DMA_ISR_TCIF3) {         // got end of transfer
        OSD_DMA->CCR &= ~DMA_CCR3_EN;
        DMA1->IFCR |= DMA_ISR_TCIF3;
        while(OSD_SPI->SR & SPI_SR_BSY);    // wait SPI for last bits

    	//Disable the timer
        TIM_Cmd(TIM3, DISABLE);             // shut up my dear sck
        //Update the timer to reset internal counters
        TIM_GenerateEvent( TIM3, TIM_EventSource_Update );
        //Prevent it from being triggered again
        TIM_SelectSlaveMode( TIM3, 0 );

        // clear prev OSD_RAM line when that's enabled
        if ( osdData.ptrLine ) {
            DMA1_Channel1->CCR &= (uint16_t)(~DMA_CCR1_EN);
            DMA1_Channel1->CMAR = (uint32_t)osdData.ptrLine;
            DMA1_Channel1->CNDTR = OSD_HRES;
            DMA1_Channel1->CCR |= DMA_CCR1_EN;
        }

    } 
}

void TIM1_UP_IRQHandler(void) {

}

void TIM1_CC_IRQHandler(void) {

}

void TIM1_TRG_COM_IRQHandler(void) {
	static int inv = 0;

	if (TIM1 ->SR & TIM_SR_TIF ) {      // Triggered on start of hsync
		TIM_ClearFlag(TIM1, TIM_SR_TIF );
		//Check if we're already in vsync
		if ( !(GPIOB->IDR & GPIO_Pin_8 ) ) {
			//Enough lines done to restart
			if ( osdData.currentScanLine > 0) {    // wait VSYNC
				osdData.maxScanLine = osdData.currentScanLine;
				osdData.PAL = osdData.maxScanLine > 200 ? 1 : 0;			// recheck mode
				//Reset for next frame
				osdData.currentScanLine = -cfg.delayY;
				//Disable timer3
//				TIM_SelectSlaveMode( TIM3, 0 );
				//Signal the new frame to be drawn
				CoEnterISR();
				isr_SetFlag(osdData.osdUpdateFlag);	// redraw after even frame
				CoExitISR();
			}
		} else if (GPIOA ->IDR & GPIO_Pin_8 ) {   // check HSYNC
			osdData.currentScanLine++;
			//Previous line enabled tim3 timer, disable it before we start dma
			if (osdData.currentScanLine == 0) {
				//Enable timer3 again
//				TIM_SelectSlaveMode( TIM3, TIM_SlaveMode_Trigger );
//				TIM_Cmd(TIM3, DISABLE);             // shut up my dear sck
//		        TIM_GenerateEvent( TIM3, TIM_EventSource_Update );

//				TIM_SetCounter( TIM3, 0 );
				//Update the timer to reset internal counters

				//Disable the timer
//				TIM3->CR1 &= (uint16_t) (~((uint16_t) TIM_CR1_CEN ));
				//                TIM_Cmd(TIM3, DISABLE);             // shut up my dear sck
				//Update the timer to reset internal counters
				//                TIM_GenerateEvent( TIM3, TIM_EventSource_Update );
			}
			//Check if we're in the active video output stage yet
			if (osdData.currentScanLine >= 0 && osdData.currentScanLine < cfg.height ) {
				const uint16_t dmaCount = (cfg.width >> 3) + 1;
				osdData.ptrLine = (uint8_t *) &osdData.OSD_RAM[OSD_HRES * osdData.currentScanLine ];

				// SPI1 DMA out pixels
				OSD_DMA ->CCR &= (uint16_t) (~DMA_CCR1_EN );
				OSD_DMA ->CMAR = (uint32_t) osdData.ptrLine;
				OSD_DMA ->CNDTR = dmaCount;
				OSD_DMA ->CCR |= DMA_CCR1_EN | DMA_CCR1_TCIE;
				// SPI2 DMA out BW
				OSDBW_DMA ->CCR &= (uint16_t) (~DMA_CCR1_EN );
				OSDBW_DMA ->CMAR = (uint32_t) &osdData.OSD_LINEBW;
				OSDBW_DMA ->CNDTR = dmaCount;
				OSDBW_DMA ->CCR |= DMA_CCR1_EN;

				//Enable tim3 to be triggered
				TIM_SelectSlaveMode( TIM3, TIM_SlaveMode_Trigger );
			}
			//Don't let the dma finished handler clear the line just yet
			if (!inv) {
				//                	osdData.ptrLine = 0;
			}
			// Passed last frame, redraw time
			if (osdData.currentScanLine == cfg.height) {     // max line
				inv ^= 1;
				//                if (inv) {      // no need to redraw each frame
				//osdClearScreen();
//				if ( !inv ) isr_SetFlag(osdData.osdUpdateFlag);	// redraw after even frame

				//                } else {
				//                    isr_SetFlag(osdData.osdRecalcFlag);   // recalc after odd frame
				//               }
			}
		}
	}
}

