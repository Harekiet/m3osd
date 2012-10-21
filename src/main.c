#include "board.h"
#include "osdcore.h"
#include "sensors.h"
#include "uart.h"
#include "gps.h"
#include "multiwii.h"
#include "usb.h"
#include "fonts.h"

#define BLINK_STACK_SIZE 48
OS_STK blinkStack[BLINK_STACK_SIZE];
volatile uint32_t minCycles, idleCounter, totalCycles;
uint32_t oldIdleCounter;
volatile float idlePercent;

void blinkTask(void *unused)
{
    while (1) {
        CoTickDelay(250);
        LED0_TOGGLE;
        idlePercent = 100.0f * (idleCounter - oldIdleCounter) * minCycles / totalCycles;
        oldIdleCounter = idleCounter;
        totalCycles = 0;
        // gpsData.mode = MODE_PASSTHROUGH;
    }
}

void CoIdleTask(void *pdata)
{
    volatile unsigned long cycles;
    volatile unsigned int *DWT_CYCCNT = (volatile unsigned int *) 0xE0001004;
    volatile unsigned int *DWT_CONTROL = (volatile unsigned int *) 0xE0001000;
    volatile unsigned int *SCB_DEMCR = (volatile unsigned int *) 0xE000EDFC;

    *SCB_DEMCR = *SCB_DEMCR | 0x01000000;
    *DWT_CONTROL = *DWT_CONTROL | 1;    // enable the counter

    minCycles = 99999999;
    while (1) {
        idleCounter++;

        __nop();
        __nop();
        __nop();
        __nop();

        cycles = *DWT_CYCCNT;
        *DWT_CYCCNT = 0;        // reset the counter

        // record shortest number of instructions for loop
        totalCycles += cycles;
        if (cycles < minCycles)
            minCycles = cycles;
    }
}

void CoStkOverflowHook(OS_TID taskID)
{
    // Process stack overflow here
    while (1);
}

void setup(void)
{
    GPIO_InitTypeDef gpio;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    // turn on peripherals needed by all
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    // all ain
    gpio.GPIO_Pin = GPIO_Pin_All;
    gpio.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &gpio);
    GPIO_Init(GPIOB, &gpio);
    GPIO_Init(GPIOC, &gpio);

    // Debug LED
    gpio.GPIO_Pin = LED0_PIN | LED1_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(LED0_GPIO, &gpio);
    LED0_OFF;
    LED1_OFF;

    // delay/timing timer configuration (TIM4)
    TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;
    TIM_TimeBaseStructure.TIM_Period = 0xFFFF;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
    TIM_Cmd(TIM4, ENABLE);
}

// microsecond delay using TIM4
void timerDelay(uint16_t us)
{
    uint16_t cnt = TIM4->CNT;
    uint16_t targetTimerVal = cnt + us;

    if (targetTimerVal < cnt)
        // wait till timer rolls over
        while (TIM4->CNT > targetTimerVal);

    while (TIM4->CNT < targetTimerVal);
}

void osdHorizon(void)
{
    int x_dim = OSD_WIDTH;      // Number of screen pixels along x axis
    int y_dim = OSD_HEIGHT;     // Number of screen pixels along y axis
    int L = 79;                 // Length of main horizon line indicator
    int l = 10;                 // Length of small angle indicator lines
    float theta_max = 60 * (M_PI / 180);        // Max pitch angle displayed on screen
    int x_c, y_c;
    int x_a[12] = { 0, };
    int y_a[12] = { 0, };
    float pitch;
    float roll;
    float d1, d2, alpha1, alpha2;
    int i, idx = 0, p, r;
    int flag = 5;               // was 7
    int x_c2, y_c2;
    int L2 = 49;

    pitch = multiwiiData.anglePitch * (M_PI / 180.0f);
    roll = multiwiiData.angleRoll * (M_PI / 180.0f);

    x_c = x_dim / 2;
    y_c = y_dim / 2 * (1 - pitch / theta_max);
    x_c2 = x_dim / 2 + sinf(roll) * flag;
    y_c2 = y_dim / 2 * (1 - pitch / theta_max) + cosf(roll) * flag;

    for (i = 20; i <= 60; i += 20) {
        d1 = sqrtf(17 * 17 + powf(i * M_PI / 180.0f / theta_max * y_dim / 2, 2));
        alpha1 = atan2f((i * M_PI / 180.0f / theta_max * y_dim / 2), 17);
        // d2 = sqrtf(50 * 50 + powf(i * M_PI / 180 / theta_max * y_dim / 2, 2));
        // alpha2 = atanf((i * M_PI / 180 / theta_max * y_dim / 2) / 50);

        x_a[idx] = x_dim / 2 + d1 * cosf(alpha1 - roll);
        y_a[idx++] = (y_dim / 2) * (1 - pitch / theta_max) + d1 * sinf(alpha1 - roll);
        x_a[idx] = x_dim / 2 - d1 * cosf(alpha1 + roll);
        y_a[idx++] = (y_dim / 2) * (1 - pitch / theta_max) + d1 * sinf(alpha1 + roll);
        x_a[idx] = x_dim / 2 + d1 * cosf(alpha1 + roll);
        y_a[idx++] = (y_dim / 2) * (1 - pitch / theta_max) - d1 * sinf(alpha1 + roll);
        x_a[idx] = x_dim / 2 - d1 * cosf(alpha1 - roll);
        y_a[idx++] = (y_dim / 2) * (1 - pitch / theta_max) - d1 * sinf(alpha1 - roll);
    }

    for (i = 0; i < 12; i++) {
        osdDrawLine(x_a[i] - l * cosf(roll), y_a[i] - l * sinf(-roll), x_a[i] + l * cosf(roll), y_a[i] + l * sinf(-roll), 1);
    }

    // cross
    osdDrawHorizontalLine(x_dim / 2 - 5, y_dim / 2, 11, 1);
    osdDrawVerticalLine(x_dim / 2, y_dim / 2 - 5, 11, 1);
    osdDrawCircle(x_dim / 2, y_dim / 2, 39, 1, 0xFF);

    // horizont
    osdDrawLine(x_c - L * cosf(roll), y_c - L * sinf(-roll), x_c + L * cosf(roll), y_c + L * sinf(-roll), 1);

    // sub horizont
    osdDrawLine(x_c2 - L2 * cosf(roll), y_c2 - L2 * sinf(-roll), x_c2 + L2 * cosf(roll), y_c2 + L2 * sinf(-roll), 1);
}

//const char *string =  "QWERTYUIOPASDFGHJKLZXVBNM";
//const char *string =   "ABCDEFGHIJKL";
//const char *string =   "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const char *string = "ABCDEFGHIJKLMNOPQRSTU";
const char *stringl = "abcdefghijklmnopqrstu";

/*
  	 ! 	" 	# 	$ 	 % 	& 	' 	( 	) 	* 	+ 	, 	— 	. 	/
3. 	0 	1 	2 	3 	4 	5 	6 	7 	8 	9 	 : 	 ; 	< 	= 	> 	 ?
4. 	@ 	A 	B 	C 	D 	E 	F 	G 	H 	I 	J 	K 	L 	M 	N 	O
5. 	P 	Q 	R 	S 	T 	U 	V 	W 	X 	Y 	Z 	[ 	\ 	] 	^ 	_
6. 	` 	a 	b 	c 	d 	e 	f 	g 	h 	i 	j 	k 	l 	m 	n 	o
7. 	p 	q 	r 	s 	t 	u 	v 	w 	x 	y 	z 	{ 	| 	} 	~ 	DEL
*/

const char *numbers = "0123456789:;<=>?";
const char *numbers2 = "!\"#$%&'()*+,-./_\\{}|~`";
const char *gay = "GAY BASINGSTOKE";
const char *huh = "HELLOW PAL";

#define MAIN_STACK_SIZE 256
OS_STK mainStack[MAIN_STACK_SIZE];

void mainTask(void *unused)
{
    int i, j;
    int offset, head, alt;

    while (1) {
        //osdClearScreen(); 
        // wait for vsync
        //CoWaitForSingleFlag(osdData.osdUpdateFlag, 0);
        //CoClearFlag(osdData.osdUpdateFlag);
        //LED1_TOGGLE;

        // CoTickDelay(100);
        //osdClearScreen();
        //CoWaitForSingleFlag(osdData.osdClear, 0);
        //CoClearFlag(osdData.osdClear);
        //CoTickDelay(1);



        CoWaitForSingleFlag(osdData.osdUpdateFlag, 0);
        CoClearFlag(osdData.osdUpdateFlag);
        LED1_TOGGLE;

        //osdDrawRectangle(0, 0, OSD_WIDTH - 8, OSD_HEIGHT, 1);

        osdHorizon();

        // heading 
        osdDrawHorizontalLine(100, 14, 200, 1);
        // osdDrawVerticalLine(200,10,12,1);
        osdSetCursor(200 - 3, 12);
        //osdDrawCharacter(247+32,4);
        //osdDrawCharacter(253+32,4);
        osdDrawCharacter(95 + 32, FONT_16PX_FIXED);
        for (i = 10; i < 190; i += 10) {
            head = multiwiiData.heading;
            offset = head % 10;
            //if (!(i % 5)) 
            osdDrawVerticalLine(100 + i - offset, 9, 5, 1);
            //else 
            //    osdDrawVerticalLine(100 + i - offset, 9 + 3, 2, 1);

            switch (head - offset - 100 + i) {
            case 0:
                osdSetCursor(100 + i - offset - 3, 0);
                osdDrawCharacter('N', FONT_8PX_FIXED);
                break;
            case 90:
                osdSetCursor(100 + i - offset - 3, 0);
                osdDrawCharacter('E', FONT_8PX_FIXED);
                break;
            case 180:
            case -180:
                osdSetCursor(100 + i - offset - 3, 0);
                osdDrawCharacter('S', FONT_8PX_FIXED);
                break;
            case -90:
                osdSetCursor(100 + i - offset - 3, 0);
                osdDrawCharacter('W', FONT_8PX_FIXED);
                break;
            }

        }
        osdSetCursor(OSD_WIDTH / 2 - 2 * 8, 25);
        osdDrawDecimal(FONT_16PX_FIXED, multiwiiData.heading, 3, 0, -1);



        // altitude
        osdDrawVerticalLine(OSD_WIDTH - 100 + 20 + 8, 50, 160, 1);
        osdSetCursor(OSD_WIDTH - 100 - 1 + 20, OSD_HEIGHT / 2 - 7);
        // osdDrawCharacter(239+32,4);
        osdDrawCharacter(249 + 32, FONT_16PX_FIXED);
        for (i = 50; i < 210; i += 10) {
            alt = multiwiiData.altitude / 100;
            //alt = multiwiiData.GPS_altitude;
            offset = alt % 10;
            osdDrawHorizontalLine(OSD_WIDTH - 100 + 20 + 8, i + offset - 3, 5, 1);
            if (!((alt - offset + 120 - i) % 20) && (alt - offset + 120 - i + 10 < alt || alt - offset + 120 - i - 10 > alt)) {
                osdSetCursor(OSD_WIDTH - 90 + 20, i + offset - 6);
                osdDrawDecimal(FONT_16PX_FIXED, alt - offset + 120 - i, 5, 0, 5);
            }

        }
        osdSetCursor(OSD_WIDTH - 98 + 20 + 8, OSD_HEIGHT / 2 - 7);
        //osdDrawDecimal(4, multiwiiData.GPS_altitude, 4, 0, 4);
        osdDrawDecimal(4, multiwiiData.altitude, 5, 0, 2);


        /*
           osdDrawCircle(50, 150, 32, 1, 0xFF);
           osdDrawFilledCircle(50, 150, 16, 1, 0xFF);
           osdDrawFilledCircle(50, 150, 8, 0, GFX_QUADRANT0 | GFX_QUADRANT2);
         */

        //osdDrawRectangle(0, 0, OSD_WIDTH - 8, OSD_HEIGHT, 1);


        osdSetCursor(OSD_WIDTH - 8 * 8, 2);
        osdDrawCharacter('L', FONT_8PX_FIXED);
        osdDrawCharacter('N', FONT_8PX_FIXED);
        osdDrawDecimal(FONT_8PX_FIXED, osdData.maxScanLine, 3, 0, 3);
        if (osdData.PAL) {
            osdSetCursor(OSD_WIDTH - 8 * 8, 2 + 8);
            osdDrawCharacter('P', FONT_8PX_FIXED);
            osdDrawCharacter('A', FONT_8PX_FIXED);
            osdDrawCharacter('L', FONT_8PX_FIXED);
        } else {
            osdSetCursor(OSD_WIDTH - 8 * 8, 2 + 8);
            osdDrawCharacter('N', FONT_8PX_FIXED);
            osdDrawCharacter('T', FONT_8PX_FIXED);
            osdDrawCharacter('S', FONT_8PX_FIXED);
            osdDrawCharacter('C', FONT_8PX_FIXED);
        }

        // GPS
        osdSetCursor(3, 50);
        osdDrawCharacter('F', FONT_16PX_FIXED);
        osdDrawCharacter('i', FONT_16PX_FIXED);
        osdDrawCharacter('x', FONT_16PX_FIXED);
        osdDrawDecimal(FONT_16PX_FIXED, multiwiiData.GPS_FIX, 1, 0, 1);

        osdSetCursor(3, 50 + 16);
        osdDrawCharacter('S', FONT_16PX_FIXED);
        osdDrawCharacter('a', FONT_16PX_FIXED);
        osdDrawCharacter('t', FONT_16PX_FIXED);
        osdDrawDecimal(FONT_16PX_FIXED, multiwiiData.GPS_numSat, 2, 0, 2);

        // int32_t GPS_LAT;
        osdSetCursor(0, 0);
        osdDrawCharacter(multiwiiData.GPS_LAT < 0 ? 'S' : 'N', FONT_8PX_FIXED);      // S
        osdDrawDecimal(FONT_8PX_FIXED, abs(multiwiiData.GPS_LAT), 9, 1, 7);

        // int32_t GPS_LON;
        osdSetCursor(0, 0 + 10);
        osdDrawCharacter(multiwiiData.GPS_LON < 0 ? 'W' : 'E', FONT_8PX_FIXED);      // W
        osdDrawDecimal(FONT_8PX_FIXED, abs(multiwiiData.GPS_LON), 9, 1, 7);

        // int32_t GPS_homeLAT;
        osdSetCursor(0, +20);
        osdDrawCharacter(multiwiiData.GPS_homeLAT < 0 ? 'S' : 'N', FONT_8PX_FIXED);  // S
        osdDrawDecimal(FONT_8PX_FIXED, abs(multiwiiData.GPS_homeLAT), 9, 1, 7);

        // int32_t GPS_homeLON;
        osdSetCursor(0, 0 + 30);
        osdDrawCharacter(multiwiiData.GPS_homeLON < 0 ? 'W' : 'E', FONT_8PX_FIXED);  // W
        osdDrawDecimal(FONT_8PX_FIXED, abs(multiwiiData.GPS_homeLON), 9, 1, 7);




        osdSetCursor(3, 50 + 16 * 2);
        osdDrawCharacter('A', FONT_16PX_FIXED);
        osdDrawCharacter('l', FONT_16PX_FIXED);
        osdDrawCharacter('G', FONT_16PX_FIXED);
        osdDrawDecimal(FONT_16PX_FIXED, multiwiiData.GPS_altitude, 5, 0, 5);  // altitude in 0.1m

        osdSetCursor(3, 50 + 16 * 3);
        osdDrawCharacter('S', FONT_16PX_FIXED);
        osdDrawCharacter('p', FONT_16PX_FIXED);
        osdDrawCharacter('G', FONT_16PX_FIXED);
        osdDrawDecimal(FONT_16PX_FIXED, multiwiiData.GPS_speed /**360/100000*/ , 5, 0, 5);    // speed in sm/sec

        osdSetCursor(3, 50 + 16 * 4);
        osdDrawCharacter('u', FONT_16PX_FIXED);
        //osdDrawDecimal(1, multiwiiData.GPS_update, 3, 3, 3);
        //osdDrawDecimal(2, multiwiiData.GPS_update, 3, 3, 3);
        //osdDrawDecimal(3, multiwiiData.GPS_update, 3, 3, 3);
        osdDrawDecimal(FONT_16PX_FIXED, multiwiiData.GPS_update, 3, 3, 3);
        osdDrawDecimal(FONT_8PX_FIXED, multiwiiData.GPS_update, 3, 3, 3);

        osdSetCursor(3, 50 + 16 * 5);
        osdDrawCharacter('L', FONT_16PX_FIXED);
        osdDrawCharacter('O', FONT_16PX_FIXED);
        osdDrawCharacter('S', FONT_16PX_FIXED);
        osdDrawDecimal(FONT_16PX_FIXED, multiwiiData.GPS_distanceToHome, 5, 0, 5);    // meters?


        osdSetCursor(3, 50 + 16 * 6);
        osdDrawCharacter('H', FONT_16PX_FIXED);
        osdDrawCharacter('O', FONT_16PX_FIXED);
        osdDrawCharacter('D', FONT_16PX_FIXED);
        osdDrawDecimal(FONT_16PX_FIXED, multiwiiData.GPS_directionToHome, 5, 0, 5);   // ??



        osdSetCursor(3, OSD_HEIGHT - 2 * 16 - 1);
        osdDrawDecimal(FONT_16PX_FIXED, 1260, 5, 0, 2);
        osdDrawCharacter('V', FONT_16PX_FIXED);
        osdDrawDecimal(FONT_16PX_FIXED, 1234, 5, 0, 2);
        osdDrawCharacter('A', FONT_16PX_FIXED);


        osdSetCursor(3, OSD_HEIGHT - 1 * 16 - 1);
        osdDrawDecimal(FONT_16PX_FIXED, 1234, 5, 0, 5);
        osdDrawCharacter('M', FONT_16PX_FIXED);
        osdDrawCharacter('a', FONT_16PX_FIXED);
        osdDrawCharacter('h', FONT_16PX_FIXED);

/*
        osdSetCursor(50, 30);
        osdDrawDecimal(2, sensorData.batteryData.voltage[0] * 100, 3, 1, 2);
        osdDrawCharacter('V', 2);
        osdSetCursor(50, 56);
        osdDrawDecimal(2, sensorData.volts * 100, 4, 1, 2);
        osdDrawCharacter('V', 2);
*/
        osdSetCursor(50, 185);
        osdDrawDecimal(FONT_8PX_PROP, multiwiiData.anglePitch, 3, 0, -1);
        osdSetCursor(80, 185);
        osdDrawDecimal(FONT_8PX_PROP, multiwiiData.angleRoll, 3, 0, -1);
/*				
				osdSetCursor(50, 200);
        osdDrawDecimal(1, multiwiiData.anglePitch, 5, 0, 3);
        osdSetCursor(80, 200);
        osdDrawDecimal(1, multiwiiData.angleRoll, 5, 0, 3);
				*/

        //CoTickDelay(2);
    }
}

int main(void)
{
    int i = 0, j = 0, q = 0;
    int x = 0, y = 0, xdir = 1, ydir = 1;

    setup();
    CoInitOS();

    USB_Renumerate();
    USB_Interrupts_Config();
    USB_Init();

    osdInit();
    sensorsInit();
    gpsInit();
    multiwiiInit();

    CoCreateTask(blinkTask, 0, 30, &blinkStack[BLINK_STACK_SIZE - 1], BLINK_STACK_SIZE);
    CoCreateTask(mainTask, 0, 10, &mainStack[MAIN_STACK_SIZE - 1], MAIN_STACK_SIZE);
    // minCycles = 99999999;
    CoStartOS();

#if 0
    for (i = 0; i < strlen(numbers); i++)
        osdDrawCharacter(numbers[i], 0);


    for (j = 0; j < 20; j++) {
        osdSetCursor(0 + j * (rand() % 3), 10 + j * 8);

        for (i = 0; i < strlen(string); i++)
            osdDrawCharacter(string[i], 1);
    }

    for (j = 0; j < 10; j++) {
        osdSetCursor(32 + j * (rand() % 3), 80 + j * 8);
        for (i = 0; i < strlen(string); i++)
            osdDrawCharacter(string[i], 2);
    }

    for (j = 0; j < 20; j++) {
        osdSetCursor(200 + j * (rand() % 3), 10 + j * 8);

        for (i = 0; i < strlen(string); i++)
            osdDrawCharacter(string[i], 1);
    }

    osdSetCursor(20, 50);
    osdDrawDecimal(2, 99999, 5, 1, 2);
    osdDrawCharacter('V', 2);

    osdSetCursor(20, 80);
    osdDrawDecimal(2, 1234, 6, 1, 2);
    // mainvector();
    // osdDrawPixel(0, 0, 1);
    // osdDrawPixel(100, 0, 1);
    // osdDrawPixel(200, 0, 1);

    x = 200;
    y = 100;

    i = 0;
    j = 0;
    q = 0;

    while (1) {
        // wait for vsync
        LED0_ON;
        CoTickDelay(10);
        // CoWaitForSingleFlag(osdData.osdUpdateFlag, 0); // wait forever
        LED0_OFF;
        CoClearFlag(osdData.osdUpdateFlag);
        osdClearScreen();

        osdSetCursor(50, 20);
        osdDrawDecimal(2, multiwiiData.altitude, 5, 0, 2);

        osdSetCursor(50, 30);
        osdDrawDecimal(2, sensorData.batteryData.voltage[0] * 100, 3, 1, 2);
        osdDrawCharacter('V', 2);
        osdSetCursor(50, 56);
        osdDrawDecimal(2, sensorData.volts * 100, 4, 1, 2);
        osdDrawCharacter('V', 2);

        osdSetCursor(50, 180);
        osdDrawDecimal(1, multiwiiData.anglePitch, 5, 0, 3);
        osdSetCursor(80, 180);
        osdDrawDecimal(1, multiwiiData.angleRoll, 5, 0, 3);

        // artificial horizon
        // osdHorizon(); 
        // osdDrawLine(100, 100 - multiwiiData.anglePitch + multiwiiData.angleRoll, 300, 100 - multiwiiData.anglePitch - multiwiiData.angleRoll, 1);
        // osdSetCursor(10, 20);
        // osdDrawDecimal(1, , 6, 0, 2);

        osdDrawHorizontalLine(100, 10, 200, 1);
        j = 0;
        for (i = 0; i < 200; i += 10) {
            int offset = (multiwiiData.heading % 10) * 2;
            osdDrawVerticalLine(100 + i + offset, 10, j ? 10 : 5, 1);
            j = !j;
        }

#if 1
        // osdDrawRectangle(0, 0, 399, 199, 1);

        if (x > 399)
            xdir = -1;
        if (x < 1)
            xdir = 1;
        if (y > 170)
            ydir = -1;
        if (y < 20)
            ydir = 1;

        x += xdir;
        y += ydir;

        osdDrawRectangle(x - 4, y - 4, 8, 8, 1);
#if 0
        osdDrawFilledCircle(x, y, 16, 1, 0xFF);
        if (q++ >> 4 & 1)
            osdDrawFilledCircle(x, y, 8, 0, GFX_QUADRANT0 | GFX_QUADRANT2);
        else
            osdDrawFilledCircle(x, y, 8, 0, GFX_QUADRANT1 | GFX_QUADRANT3);
#endif
        // osdSetCursor(x, y);
        // osdDrawDecimal(2, i, 5, 1, 2);

        // osdSetCursor(20, 30);
        // osdDrawDecimal(2, 137, 5, 0, 2);
        // i++;
#endif
    }
#endif
}
