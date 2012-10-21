#pragma once

/* Hardware resources usage

    >> Timers
    TIM1_CH1        HSYNC input for OSD

    TIM2_CH1        LED0 PWM output (PA0)
    TIM2_CH2        LED1 PWM output (PA1)

    TIM3_CH3        PWM1 input (for gimbal attitude input)
    TIM3_CH4        PWM2 input

    TIM4_CH2        ODD/EVEN input for OSD (unused currently, rev2 pcb only)
    TIM4_CH3        VSYNC input for OSD (unused currently)

    >> LED
    PA0             LED0 (gpio or PWM controlled)
    PA1             LED1 (gpio or PWM controlled)

    >> Pushbutton
    PC13            S1 (momentary button)
    PC14            S2 (momentary button)

    >> ADC
    PA4             ADC12_IN4 power voltage divider
    PA5             ADC12_IN5 RSSI input

    >> GPIO
    PA15            USB connection detect
    PB12            CONF1 jumper
    PB13            CONF2 jumper

    >> USART
    USART1          GPS/emergency firmware update
    USART2          telemetry input from MWC/OP/etc
    USART3          Flexiport usart

*/

// for roundf()
#define __USE_C99_MATH

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <CoOS.h>

#include "stm32f10x_conf.h"
#include "core_cm3.h"

#ifndef M_PI
#define M_PI 3.1415926f
#endif

#define digitalHi(p, i)     { p->BSRR = i; }
#define digitalLo(p, i)     { p->BRR = i; }
#define digitalToggle(p, i) { p->ODR ^= i; }

// Hardware definitions and GPIO
#define LED0_GPIO   GPIOA
#define LED0_PIN    GPIO_Pin_0
#define LED1_GPIO   GPIOA
#define LED1_PIN    GPIO_Pin_1

#define LED0_TOGGLE digitalToggle(LED0_GPIO, LED0_PIN);
#define LED0_OFF    digitalHi(LED0_GPIO, LED0_PIN);
#define LED0_ON     digitalLo(LED0_GPIO, LED0_PIN);

#define LED1_TOGGLE digitalToggle(LED1_GPIO, LED1_PIN);
#define LED1_OFF    digitalHi(LED1_GPIO, LED1_PIN);
#define LED1_ON     digitalLo(LED1_GPIO, LED1_PIN);

void timerDelay(uint16_t us);
