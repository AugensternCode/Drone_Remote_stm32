#include "led.h"
uint16_t Blink_Speed_Count = 0;		//状态RGB灯闪烁速度设置
#define BLINK_SPEED 10
uint16_t Blink_Period_Count = 0;	//状态RGB灯闪烁周期设置
#define BLINK_PERIOD 5

/* led端口初始化 */
void LedInit(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RED_GPIO_CLK | GREEN_GPIO_CLK | BLUE_GPIO_CLK,ENABLE);	
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = RED_GPIO_PIN|GREEN_GPIO_PIN|BLUE_GPIO_PIN;
    GPIO_Init(RED_GPIO_PORT, &GPIO_InitStructure); 
	GPIO_Init(GREEN_GPIO_PORT,&GPIO_InitStructure);
	GPIO_Init(BLUE_GPIO_PORT,&GPIO_InitStructure);
	LedColorSet(RED);
}

//关闭状态指示灯
static void LedStatusOff(void)
{
	GPIOB->BSRR = RED_GPIO_PIN;  //红，置为高电平，因为LED的公共极连的是阳极
	GPIOB->BSRR = GREEN_GPIO_PIN;  //绿
	GPIOB->BSRR = BLUE_GPIO_PIN;  //蓝
} 

//LED灯颜色设置
void LedColorSet(const uint8_t LedColor)
{
	LedStatusOff();	
	switch(LedColor){
		case RED:
			RGB_RED;//红
			break;
		case GREEN:
			RGB_GREEN;//绿
			break;
		case BLUE:
			RGB_BLUE;//蓝
			break;
		case YELLOW:	//黄
			RGB_RED;
			RGB_GREEN;
			break;
		case PURPLE:	//紫
			RGB_RED;
			RGB_BLUE;
			break;
		case CYAN:	//青
			RGB_GREEN;
			RGB_BLUE;
			break;
		case WHITE:	//白
			RGB_RED;
			RGB_GREEN;
			RGB_BLUE;
			break;
		default :
			LedStatusOff();	//全灭
			break;
	}
}

void LedBlink(uint8_t ledColor)
{
	if( Blink_Speed_Count < BLINK_SPEED )
	{
        Blink_Speed_Count++;
		if( Blink_Speed_Count == BLINK_SPEED )
		{
			Blink_Period_Count++;
			Blink_Speed_Count=0;
		}
	}
	if( Blink_Period_Count >= BLINK_PERIOD - 2 && Blink_Period_Count <= BLINK_PERIOD)
	{		//闪烁
		if(Blink_Period_Count == BLINK_PERIOD)
		{
			Blink_Period_Count = 0;
		}
		LedColorSet(ledColor);		
	}
	else 
	{
		LedStatusOff();
	}
}
