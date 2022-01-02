

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "tftlcd.h"
#include "key.h"
#include "spi.h"
#include "flash.h"


//要写入到25Qxx的字符串数组
const u8 text_buf[]="www.prechin.net";
#define TEXT_LEN sizeof(text_buf)


int main()
{
	u8 i=0;
	u8 key;
	u8 buf[30];
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	TFTLCD_Init();			//LCD初始化
	KEY_Init();
	EN25QXX_Init();
	
	FRONT_COLOR=BLACK;
	LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"PRECHIN STM32F1");
	LCD_ShowString(10,30,tftlcd_data.width,tftlcd_data.height,16,"www.prechin.net");
	LCD_ShowString(10,50,tftlcd_data.width,tftlcd_data.height,16,"FLASH-SPI Test");
	LCD_ShowString(10,70,tftlcd_data.width,tftlcd_data.height,16,"K_UP:Write   K_DOWN:Read");
	FRONT_COLOR=RED;
	
	while(EN25QXX_ReadID()!=EN25Q128)			//检测不到EN25Q128
	{
		printf("EN25Q128 Check Failed!\r\n");
		LCD_ShowString(10,150,tftlcd_data.width,tftlcd_data.height,16,"EN25Q128 Check Failed!  ");
	}
	printf("EN25Q128 Check Success!\r\n");
	LCD_ShowString(10,150,tftlcd_data.width,tftlcd_data.height,16,"EN25Q128 Check Success!");
		
	LCD_ShowString(10,170,tftlcd_data.width,tftlcd_data.height,16,"Write Data:");
	LCD_ShowString(10,190,tftlcd_data.width,tftlcd_data.height,16,"Read Data :");
	
	while(1)
	{
		key=KEY_Scan(0);
		if(key==KEY_UP)
		{
			EN25QXX_Write((u8 *)text_buf,0,TEXT_LEN);
			printf("发送的数据：%s\r\n",text_buf);
			LCD_ShowString(10+11*8,170,tftlcd_data.width,tftlcd_data.height,16,"www.prechin.net");
		}
		if(key==KEY_DOWN)
		{
			EN25QXX_Read(buf,0,TEXT_LEN);
			printf("接收的数据：%s\r\n",buf);
			LCD_ShowString(10+11*8,190,tftlcd_data.width,tftlcd_data.height,16,buf);
		}
		
		i++;
		if(i%20==0)
		{
			led1=!led1;
		}
		
		delay_ms(10);
			
	}
}
