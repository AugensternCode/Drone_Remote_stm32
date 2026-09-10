#ifndef _led_h_
#define _led_h_

#include "stm32f10x.h"
//ºìµÆ-ºê¶¨Òå
#define RED_GPIO_PORT GPIOB
#define RED_GPIO_CLK RCC_APB2Periph_GPIOB
#define RED_GPIO_PIN GPIO_Pin_7
//ÂÌµÆ-ºê¶¨Òå
#define GREEN_GPIO_PORT GPIOB
#define GREEN_GPIO_CLK RCC_APB2Periph_GPIOB
#define GREEN_GPIO_PIN GPIO_Pin_8
//À¶µÆ-ºê¶¨Òå
#define BLUE_GPIO_PORT GPIOB
#define BLUE_GPIO_CLK RCC_APB2Periph_GPIOB
#define BLUE_GPIO_PIN GPIO_Pin_9

#define	RGB_RED			GPIOB->BRR = RED_GPIO_PIN
#define	RGB_GREEN		GPIOB->BRR = GREEN_GPIO_PIN
#define	RGB_BLUE		GPIOB->BRR = BLUE_GPIO_PIN

typedef enum{
	RED = 1,	//ºì		
	GREEN,		//ÂÌ
	BLUE,			//À¶
	YELLOW,		//»Æ
	PURPLE,		//×Ï
	CYAN,			//Çà
	WHITE,		//°×
}LedColor;
/*   LED2          LED1   */ 
    /** *   /|\   * * *
         *   |   *
          *  |  *
           * | *
            * *
             *
            * *
           *   *
          *     *
         *       *
    * * *         * * */
/*  LED3            LED4   */ 
void LedInit(void);
static void LedStatusOff(void);
void LedColorSet(const uint8_t LedColor);
void LedBlink(uint8_t ledColor);
#endif
