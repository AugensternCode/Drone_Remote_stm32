/*
 * ?????LED ?????????????????????????????
 */
#include "led.h"
//状态RGB灯闪烁速度设置
uint16_t blinkSpeedCount = 0;		
//状态RGB灯闪烁周期设置
uint16_t blinkPeriodCount = 0;	
//右下灯闪烁速度和周期设置
uint16_t ledButtomCount = 0;	
uint8_t LedButtomCount = 0;
//底部三个定位灯闪烁速度和周期设置
uint16_t locateLedBlinkSpeedCount = 0;	
uint16_t locateLedBlinkSpeed = 0;

/* led端口初始化 */
void RGB_LedInit(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO,ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE); 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_Init(GPIOB, &GPIO_InitStructure);  
	
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  //可删除，为了保持代码健壮性保留
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_15 | GPIO_Pin_0;
    GPIO_Init(GPIOA, &GPIO_InitStructure);  
	
	TopLedColorSet(RED);
	ButtomLedColorSet(WHITE);
}

//机头定位LED灯闪烁设置
//该逻辑为locateLedBlinkSpeedCount满10，locateLedBlinkSpeed进1，并且满5清0
//locateLedBlinkSpeed值为0，1，2的时候亮灯，3，4，5熄灯
//进而实现闪烁
void FrontLedBlinkSet(void)	
{
	if( locateLedBlinkSpeedCount < 10 ){
        locateLedBlinkSpeedCount++;
		if( locateLedBlinkSpeedCount == 10 ){
			locateLedBlinkSpeed++;
			locateLedBlinkSpeedCount=0;
		}
	}
	if( locateLedBlinkSpeed >= 3 && locateLedBlinkSpeed <= 5)
	{	//闪烁
		if(locateLedBlinkSpeed == 5)
		{
			locateLedBlinkSpeed = 0;
		}
		GPIOA->BSRR = GPIO_Pin_0;
		GPIOB->BSRR = GPIO_Pin_12;
		GPIOA->BSRR = GPIO_Pin_15;
	} 
	else 
	{
		GPIOA->BRR = GPIO_Pin_0;  //开启PA0-LED1
		GPIOB->BRR = GPIO_Pin_12; //开启PA15-LED3
		GPIOA->BRR = GPIO_Pin_15; //开启PB12-LED2
	}
}

//关闭状态指示灯
void LedStatusOff(void)
{
	GPIOA->BSRR = GPIO_Pin_12; //LED10
	GPIOB->BSRR = GPIO_Pin_13; //LED8
	GPIOB->BSRR = GPIO_Pin_14; //LED9
}
/*
红色 + 绿色 = 黄色
绿色 + 蓝色 = 青色
红色 + 蓝色 = 品红色
红色 + 绿色 + 蓝色 = 白色。
*/
//LED灯颜色设置
void TopLedColorSet(const uint8_t LedColor)
{
	LedStatusOff();
	switch(LedColor){
		case RED:
			TOP_RGB_RED;//红
			break;
		case GREEN:
			TOP_RGB_GREEN;//绿
			break;
		case BLUE:
			TOP_RGB_BLUE;//蓝
			break;
		case YELLOW:	//黄
			TOP_RGB_RED;
			TOP_RGB_GREEN;
			break;
		case PURPLE:	//紫
			TOP_RGB_RED;
			TOP_RGB_BLUE;
			break;
		case CYAN:	//青
			TOP_RGB_GREEN;
			TOP_RGB_BLUE;
			break;
		case WHITE:	//白
			TOP_RGB_RED;
			TOP_RGB_GREEN;
			TOP_RGB_BLUE;
			break;
		default :
			LedStatusOff();	//全灭
			break;
	}
}

//状态RGB灯闪烁设置
void RGB_LedBlink(uint8_t ledColor)
{
	if( blinkSpeedCount < BLINK_SPEED ){
        blinkSpeedCount++;
		if( blinkSpeedCount == BLINK_SPEED ){
			blinkPeriodCount++;
			blinkSpeedCount=0;
		}
	}
	if( blinkPeriodCount >= BLINK_PERIOD - 2 && blinkPeriodCount <= BLINK_PERIOD){		//闪烁
		if(blinkPeriodCount == BLINK_PERIOD){
			blinkPeriodCount = 0;
		}
		TopLedColorSet(ledColor);	
	}
	else {
		LedStatusOff();
	}
}

//状态RGB灯
void RGB_LedStatus(const PlaneData plane)
{
	if(plane.pair == PAIR_NOT){   //配对状态如果未配对0
		TopLedColorSet(RED); //顶部LED亮红灯
	}
	else if(plane.signal == SIGNAL_LOST){ //信号状态丢失
		RGB_LedBlink(BLUE); //RGB状态指示灯亮蓝灯
	}
	else if(plane.lock == UNLOCK){ //如果上锁状态未锁定
		TopLedColorSet(GREEN); //顶部指示灯亮绿灯
	}
	else if(plane.signal == SIGNAL_NORMAL){ //信号状态正常
		TopLedColorSet(CYAN); //亮青灯
	}

	if(plane.power == POWER_LOWER){ //电量低
		FrontLedBlinkSet();  //机头灯闪烁
	}else {
		GPIOA->BRR = GPIO_Pin_0;  //LED1
		GPIOB->BRR = GPIO_Pin_12; //LED2
		GPIOA->BRR = GPIO_Pin_15; //LED3
	}
}

//关闭右下角RGB灯
void ButtomLedOff(void)
{
	GPIOB->BSRR = GPIO_Pin_7;
	GPIOB->BSRR = GPIO_Pin_8;
	GPIOB->BSRR = GPIO_Pin_9;
}

//LED灯颜色设置
void ButtomLedColorSet(const uint8_t LedColor)
{
	ButtomLedOff();
	switch(LedColor){
		case RED:
			BUTTOM_RGB_RED;//红
			break;
		case GREEN:
			BUTTOM_RGB_GREEN;//绿
			break;
		case BLUE:
			BUTTOM_RGB_BLUE;//蓝
			break;
		case YELLOW:	//黄
			BUTTOM_RGB_RED;
			TOP_RGB_GREEN;
			break;
		case PURPLE:	//紫
			BUTTOM_RGB_RED;
			BUTTOM_RGB_BLUE;
			break;
		case CYAN:	//青
			BUTTOM_RGB_GREEN;
			BUTTOM_RGB_BLUE;
			break;
		case WHITE:	//白
			BUTTOM_RGB_RED;
			BUTTOM_RGB_GREEN;
			BUTTOM_RGB_BLUE;
			break;
		default :
			ButtomLedOff();
			break;
	}
}


//右下RGB灯闪烁设置
void LeftButtomLedBlinkSet(void)	
{
	if( ledButtomCount < 10000 ){
        ledButtomCount++;
		if( ledButtomCount == 10000 ){
			LedButtomCount++;
			ledButtomCount=0;
		}
	}
	
	if( (LedButtomCount >= 15 && LedButtomCount <= 16) || (LedButtomCount >= 18 && LedButtomCount <= 19)){	//闪烁两次
			ButtomLedColorSet(WHITE);
		
		if(LedButtomCount == 19){
			LedButtomCount = 0;
		}
	}
	else {
			ButtomLedOff();
	}
	
}

