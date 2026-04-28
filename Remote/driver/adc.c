#include "adc.h"
#include "led.h"
#include "usart1.h"
#include "nrf24l01.h"




uint16_t ADC_value[4];

/* 将 PA0~PA3 配置为模拟输入，对应 4 路 ADC 采样 */
void adc_gpio_init(void)
{
	GPIO_InitTypeDef GPIO_initStructure;    
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	
	GPIO_initStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3;	    
	GPIO_initStructure.GPIO_Mode = GPIO_Mode_AIN;								    
	GPIO_Init(GPIOA,&GPIO_initStructure);	
}

/* 配置 ADC1:
 * 1. 扫描 4 个规则通道
 * 2. 由 TIM4_CH4 外部触发启动转换
 * 3. 采样结果交给 DMA 自动搬运
 */
void adc_config(void)
{
    ADC_InitTypeDef ADC_initStructure;
    
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);
	
	ADC_initStructure.ADC_ContinuousConvMode = DISABLE;					        // 单次转换模式，每次由定时器触发开始
	ADC_initStructure.ADC_DataAlign = ADC_DataAlign_Right;		                // 数据右对齐
	ADC_initStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T4_CC4;	    // TIM4_CH4 作为外部触发源
	ADC_initStructure.ADC_Mode = ADC_Mode_Independent;							
	ADC_initStructure.ADC_NbrOfChannel = 4;										// 一次顺序采样 4 个通道
	ADC_initStructure.ADC_ScanConvMode = ENABLE;		                        // 扫描模式
	ADC_Init(ADC1,&ADC_initStructure);

	ADC_Cmd(ADC1,ENABLE);
    
	ADC_DMACmd(ADC1,ENABLE);
	
	RCC_ADCCLKConfig(RCC_PCLK2_Div8);                                           // ADC 时钟分频

	// 配置规则通道顺序和每个通道的采样时间。
	ADC_RegularChannelConfig(ADC1,ADC_Channel_0,1,ADC_SampleTime_71Cycles5);	
	ADC_RegularChannelConfig(ADC1,ADC_Channel_1,2,ADC_SampleTime_71Cycles5);
	ADC_RegularChannelConfig(ADC1,ADC_Channel_2,3,ADC_SampleTime_71Cycles5);
	ADC_RegularChannelConfig(ADC1,ADC_Channel_3,4,ADC_SampleTime_71Cycles5);
	
	ADC_ResetCalibration(ADC1);	                                                // 复位校准
	while(ADC_GetCalibrationStatus(ADC1));		                                // 等待复位完成
	ADC_StartCalibration(ADC1);					                                // 启动校准
	while(ADC_GetCalibrationStatus(ADC1));		                                // 等待校准完成

	ADC_ExternalTrigConvCmd(ADC1,ENABLE);	                                    // 使能外部触发
}

void ADC_DMA_Config(void)
{
	DMA_InitTypeDef DMA_initStructure;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);
	
	DMA_initStructure.DMA_BufferSize = 4;										// 对应 4 路采样结果
	DMA_initStructure.DMA_DIR = DMA_DIR_PeripheralSRC;	                        // 传输方向：外设到内存
	DMA_initStructure.DMA_M2M = DMA_M2M_Disable;								
	DMA_initStructure.DMA_MemoryBaseAddr = (u32)ADC_value;				        
	DMA_initStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;         // 内存数据宽度为 16 bit
	DMA_initStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;						// 每搬完一个值后内存地址自增
	DMA_initStructure.DMA_Mode = DMA_Mode_Circular;								// 循环模式，持续覆盖最新结果
	DMA_initStructure.DMA_PeripheralBaseAddr = ((u32)&ADC1->DR);	            // 外设地址是 ADC 数据寄存器
	DMA_initStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;	// 外设数据宽度为 16 bit
	DMA_initStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;			// 外设地址固定不变
	DMA_initStructure.DMA_Priority = DMA_Priority_Medium;						
	DMA_Init(DMA1_Channel1,&DMA_initStructure);
    
	DMA_ClearITPendingBit(DMA1_IT_TC1);			    

	// 一轮 DMA 搬运完成后产生中断，供上层处理最新采样值。
	DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,ENABLE);	
    
	DMA_Cmd(DMA1_Channel1,ENABLE);	
}


void ADC_Config(void)
{
    // 先准备输入引脚，再配置 ADC 本体，最后打开 DMA。
    adc_gpio_init();
    adc_config();
    ADC_DMA_Config();
}










