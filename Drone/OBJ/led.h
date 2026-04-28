#ifndef _led_h_
#define _led_h_

#include "stm32f10x.h"
#include "parse_packet.h"
#include "systick.h"

/*
 * LED ?????
 *
 * ?? RGB ?????????
 * ?? RGB ????????????????????
 */

#define TOP_RGB_RED       GPIOA->BRR = GPIO_Pin_12
#define TOP_RGB_GREEN     GPIOB->BRR = GPIO_Pin_14
#define TOP_RGB_BLUE      GPIOB->BRR = GPIO_Pin_13

#define BUTTOM_RGB_RED    GPIOB->BRR = GPIO_Pin_9
#define BUTTOM_RGB_GREEN  GPIOB->BRR = GPIO_Pin_8
#define BUTTOM_RGB_BLUE   GPIOB->BRR = GPIO_Pin_7

#define BLINK_SPEED   10
#define BLINK_PERIOD  5

typedef enum
{
    RED = 1,
    GREEN,
    BLUE,
    YELLOW,
    PURPLE,
    CYAN,
    WHITE,
} LedColor;

void RGB_LedInit(void);
void LeftButtomLedBlinkSet(void);
void TopLedColorSet(uint8_t LedColor);
void LedStatusOff(void);
void RGB_LedStatus(PlaneData plane);
void RGB_LedBlink(uint8_t ledColor);
void ButtomLedColorSet(const uint8_t LedColor);

#endif
