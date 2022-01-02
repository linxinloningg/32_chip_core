#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "tftlcd.h"
#include "key.h"
#include "stm32_flash.h"
#include "iap.h"




int main()
{
	u8 t;
	u8 key;
	u16 oldcount=0;				//老的串口接收数据值
	u16 applenth=0;				//接收到的app代码长度
	u8 clearflag=0;
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(115200);
	TFTLCD_Init();			//LCD初始化
	KEY_Init();
	
	FRONT_COLOR=BLACK;
	LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"PRECHIN STM32F1");
	LCD_ShowString(10,30,tftlcd_data.width,tftlcd_data.height,16,"www.prechin.net");
	LCD_ShowString(10,50,tftlcd_data.width,tftlcd_data.height,16,"IAP Test");
	LCD_ShowString(10,70,tftlcd_data.width,tftlcd_data.height,16,"K_UP:Copy APP2FLASH");
	LCD_ShowString(10,90,tftlcd_data.width,tftlcd_data.height,16,"K_DOWN:Erase SRAM APP");
	LCD_ShowString(10,110,tftlcd_data.width,tftlcd_data.height,16,"K_LEFT:Run FLASH APP");
	LCD_ShowString(10,130,tftlcd_data.width,tftlcd_data.height,16,"K_RIGHT:Run SRAM APP");
	
	FRONT_COLOR=RED;
	
	
	while(1)
	{
		if(USART_RX_CNT)
		{
			if(oldcount==USART_RX_CNT)//新周期内,没有收到任何数据,认为本次数据接收完成.
			{
				applenth=USART_RX_CNT;
				oldcount=0;
				USART_RX_CNT=0;
				printf("用户程序接收完成!\r\n");
				printf("代码长度:%dBytes\r\n",applenth);
			}else oldcount=USART_RX_CNT;			
		}
		t++;
		delay_ms(10);
		if(t==30)
		{
			led1=!led1;
			t=0;
			if(clearflag)
			{
				clearflag--;
				if(clearflag==0)LCD_Fill(10,210,240,210+16,WHITE);//清除显示
			}
		}	  	 
		key=KEY_Scan(0);
		if(key==KEY_UP)
		{
			if(applenth)
			{
				printf("开始更新固件...\r\n");	
				LCD_ShowString(10,210,200,16,16,"Copying APP2FLASH...");
 				if(((*(vu32*)(0X20001000+4))&0xFF000000)==0x08000000)//判断是否为0X08XXXXXX.
				{	 
					iap_write_appbin(FLASH_APP1_ADDR,USART_RX_BUF,applenth);//更新FLASH代码   
					LCD_ShowString(10,210,200,16,16,"Copy APP Successed!!");
					printf("固件更新完成!\r\n");	
				}else 
				{
					LCD_ShowString(10,210,200,16,16,"Illegal FLASH APP!  ");	   
					printf("非FLASH应用程序!\r\n");
				}
 			}else 
			{
				printf("没有可以更新的固件!\r\n");
				LCD_ShowString(10,210,200,16,16,"No APP!");
			}
			clearflag=7;//标志更新了显示,并且设置7*300ms后清除显示									 
		}
		if(key==KEY_DOWN)
		{
			if(applenth)
			{																	 
				printf("固件清除完成!\r\n");    
				LCD_ShowString(10,210,200,16,16,"APP Erase Successed!");
				applenth=0;
			}else 
			{
				printf("没有可以清除的固件!\r\n");
				LCD_ShowString(10,210,200,16,16,"No APP!");
			}
			clearflag=7;//标志更新了显示,并且设置7*300ms后清除显示									 
		}
		if(key==KEY_LEFT)
		{
			printf("开始执行FLASH用户代码!!\r\n");
			if(((*(vu32*)(FLASH_APP1_ADDR+4))&0xFF000000)==0x08000000)//判断是否为0X08XXXXXX.
			{	 
				iap_load_app(FLASH_APP1_ADDR);//执行FLASH APP代码
			}else 
			{
				printf("非FLASH应用程序,无法执行!\r\n");
				LCD_ShowString(10,210,200,16,16,"Illegal FLASH APP!");	   
			}									 
			clearflag=7;//标志更新了显示,并且设置7*300ms后清除显示	  
		}
		if(key==KEY_RIGHT)
		{
			printf("开始执行SRAM用户代码!!\r\n");
			if(((*(vu32*)(0X20001000+4))&0xFF000000)==0x20000000)//判断是否为0X20XXXXXX.
			{	 
				iap_load_app(0X20001000);//SRAM地址
			}else 
			{
				printf("非SRAM应用程序,无法执行!\r\n");
				LCD_ShowString(10,210,200,16,16,"Illegal SRAM APP!");	   
			}									 
			clearflag=7;//标志更新了显示,并且设置7*300ms后清除显示	 
		}
			
	}
}
