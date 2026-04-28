/*
 * ?????OLED ???????????????????????????
 */
#include "oled.h"
#include "OledFont.h"
#include "timer.h"
#include "iic.h"
#include "led.h"



/*******************************************************************************
* 函 数 名         : OLED_Init()
* 函数功能		     : OLED初始化
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
void OLED_Init(void)
{
	IIC_Init(); 
	delay_ms(200);
	OLED_WR_Byte(0xAE,OLED_CMD);//--display off
	OLED_WR_Byte(0x00,OLED_CMD);//---set low column address0
	OLED_WR_Byte(0x10,OLED_CMD);//---set high column address16
	OLED_WR_Byte(0x40,OLED_CMD);//--set start line address  
	OLED_WR_Byte(0xB0,OLED_CMD);//--set page address176
	OLED_WR_Byte(0x81,OLED_CMD); // contract control
	OLED_WR_Byte(0x08,OLED_CMD);//--128   
	OLED_WR_Byte(0xA1,OLED_CMD);//set segment remap 
	OLED_WR_Byte(0xA6,OLED_CMD);//--normal / reverse
	OLED_WR_Byte(0xA8,OLED_CMD);//--set multiplex ratio(1 to 64)
	OLED_WR_Byte(0x3F,OLED_CMD);//--1/32 duty
	OLED_WR_Byte(0xC8,OLED_CMD);//Com scan direction
	OLED_WR_Byte(0xD3,OLED_CMD);//-set display offset
	OLED_WR_Byte(0x00,OLED_CMD);//
	
	OLED_WR_Byte(0xD5,OLED_CMD);//set osc division
	OLED_WR_Byte(0x80,OLED_CMD);//
	
	
	OLED_WR_Byte(0xD8,OLED_CMD);//set area color mode off
	OLED_WR_Byte(0x05,OLED_CMD);//
	
	OLED_WR_Byte(0xD9,OLED_CMD);//Set Pre-Charge Period
	OLED_WR_Byte(0xF1,OLED_CMD);//
	
	OLED_WR_Byte(0xDA,OLED_CMD);//set com pin configuartion
	OLED_WR_Byte(0x12,OLED_CMD);//
	
	OLED_WR_Byte(0xDB,OLED_CMD);//set Vcomh
	OLED_WR_Byte(0x30,OLED_CMD);//
	
	OLED_WR_Byte(0x8D,OLED_CMD);//set charge pump enable
	OLED_WR_Byte(0x14,OLED_CMD);//
	
	OLED_WR_Byte(0xAF,OLED_CMD);//--turn on oled panel
}



/*******************************************************************************
* 函 数 名         : Write_OLED_Command()
* 函数功能		     : OLED写命令
* 输    入         : OLED_Command(命令)
* 输    出         : 无
*******************************************************************************/
void Write_OLED_Command(unsigned char OLED_Command)
{
  IIC_Start();
  IIC_Send_Byte(0x78);       //Slave address,SA0=0
	IIC_Wait_Ack();	
  IIC_Send_Byte(0x00);			//D/C#=0;写命令
	IIC_Wait_Ack();	
  IIC_Send_Byte(OLED_Command); 
	IIC_Wait_Ack();	
  IIC_Stop();
}
//
/*******************************************************************************
* 函 数 名         : Write_OLED_Data()
* 函数功能		     : OLED写数据
* 输    入         : OLED_Data(数据)
* 输    出         : 无
*******************************************************************************/
void Write_OLED_Data(unsigned char OLED_Data)
{
  IIC_Start();
  IIC_Send_Byte(0x78);			// R/W#=0;读写地址及模式选择，这里SA=0，R/W#=0（写模式）
	IIC_Wait_Ack();	
  IIC_Send_Byte(0x40);			//D/C#=1;写数据
	IIC_Wait_Ack();	
  IIC_Send_Byte(OLED_Data);//写入数据
	IIC_Wait_Ack();	
  IIC_Stop();
}




/*******************************************************************************
* 函 数 名         : OLED_WR_Byte()
* 函数功能		     : OLED命令/数据模式选择
* 输    入         : Data(数据)
										 DataType(数据类型：数据/命令)
* 输    出         : 无
*******************************************************************************/
void OLED_WR_Byte(unsigned Data,unsigned DataType)
{
	if(DataType)
	{
   Write_OLED_Data(Data);//写数据，cmd=0

	}
	else 
	{
   Write_OLED_Command(Data);	//写命令，cmd=1
	}
}



/*******************************************************************************
* 函 数 名         : OLED_Display_On()
* 函数功能		     : 开启OLED显示 
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
void OLED_Display_On(void)
{
	OLED_WR_Byte(0X8D,OLED_CMD);  //SET DCDC命令
	OLED_WR_Byte(0X14,OLED_CMD);  //DCDC ON
	OLED_WR_Byte(0XAF,OLED_CMD);  //DISPLAY ON
}


/*******************************************************************************
* 函 数 名         : OLED_Display_Off()
* 函数功能		     : 关闭OLED显示
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
void OLED_Display_Off(void)
{
	OLED_WR_Byte(0X8D,OLED_CMD);  //SET DCDC命令
	OLED_WR_Byte(0X10,OLED_CMD);  //DCDC OFF
	OLED_WR_Byte(0XAE,OLED_CMD);  //DISPLAY OFF
}		   	


/*******************************************************************************
* 函 数 名         : OLED_Clear()
* 函数功能		     : 清屏函数,清完屏,整个屏幕是黑色的!和没点亮一样!!!
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
void OLED_Clear(void)  
{  
	u8 i,n;		    
	for(i=0;i<8;i++)  
	{  
		OLED_WR_Byte (0xb0+i,OLED_CMD);    //设置页地址（0~7）
		OLED_WR_Byte (0x00,OLED_CMD);      //设置显示位置—列低地址
		OLED_WR_Byte (0x10,OLED_CMD);      //设置显示位置—列高地址   
		for(n=0;n<128;n++)OLED_WR_Byte(0,OLED_DATA); 
	} //更新显示
}



/*******************************************************************************
* 函 数 名         : OLED_On()
* 函数功能		     : oled满屏显示
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
void OLED_On(void)  
{  
	u8 i,n;		    
	for(i=0;i<8;i++)  
	{  
		OLED_WR_Byte (0xb0+i,OLED_CMD);    //设置页地址（0~7）
		OLED_WR_Byte (0x00,OLED_CMD);      //设置显示位置—列低地址
		OLED_WR_Byte (0x10,OLED_CMD);      //设置显示位置—列高地址   
		for(n=0;n<128;n++)OLED_WR_Byte(0xff,OLED_DATA); 
	} //更新显示
}




/*******************************************************************************
* 函 数 名         : Oled_Char_Display()
* 函数功能		     : 字符串显示
* 输    入         : raw(0~3)
										 column(0~7)
										 str[]
* 输    出         : 无
*******************************************************************************/
void Oled_Char_Display(u8 raw,u8 column,u8 str[])
{  
	u8 i,n,j,c,strA[16]={0},k;
		j=0,c=0;
	for(k=0;str[k]!='\0';k++)//将字符串拆开，按顺序存入strA[]数组中
	{
		strA[k]=str[k]-' ';//F8X16[][16]数组中是以' '开始的，所以要减去这个偏移量
	}
	

	for(i=raw;i<2+raw;i++)//更新显示
	{  
		OLED_WR_Byte (0xb0+i,OLED_CMD);    //设置页地址（0~7）
		OLED_WR_Byte (0x00,OLED_CMD);      //设置显示位置—列低地址
		OLED_WR_Byte (0x10+column,OLED_CMD);      //设置显示位置—列高地址   
		
	j=0;
		for(n=0;n<k*8;n++)//列数据输入
		{
			
			
			if(i==raw)//更新显示第一页（一个字符的上半部分）
			{
			//GPIO_SetBits(Led_Port,Led_Pin);
				OLED_WR_Byte(F8X16[strA[j]][c],OLED_DATA); 
				c++;
				if(c==8&&j<16)//一个字符所占的列数为8，显示完一个字符后，换到下一个字符
				{
					j++;
					c=0;
				}
			
			
			}
			

			
			if(i==raw+1)//更新显示第二页（一个字符的下班部分）
			{
				//GPIO_ResetBits(Led_Port,Led_Pin);
				OLED_WR_Byte(F8X16[strA[j]][c],OLED_DATA); 
				c++;
					if(c==16)
				{
					j++;
					c=8;
				}
			
			}
			
		}
		
		if(j==k)//第一页显示完后，换到第二页，显示字符的下半部分
		c=8;
	} 
}
