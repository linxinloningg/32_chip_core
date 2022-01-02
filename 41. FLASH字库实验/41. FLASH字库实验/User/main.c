

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "tftlcd.h"
#include "key.h"
#include "malloc.h" 
#include "sd.h"
#include "flash.h"
#include "ff.h" 
#include "fatfs_app.h"
#include "key.h"
#include "font_show.h"


int main()
{
	u8 i=0;
	u8 key;
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	TFTLCD_Init();			//LCD初始化
	KEY_Init();
	EN25QXX_Init();				//初始化EN25Q128	 	
	my_mem_init(SRAMIN);		//初始化内部内存池
	
	FRONT_COLOR=RED;//设置字体为红色 
	while(SD_Init()!=0)
	{	
		LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"SD Card Error!");
	}
	FATFS_Init();							//为fatfs相关变量申请内存				 
  	f_mount(fs[0],"0:",1); 					//挂载SD卡 
 	f_mount(fs[1],"1:",1); 				//挂载FLASH.
		
	LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"Font Test     ");	
	LCD_ShowString(10,30,tftlcd_data.width,tftlcd_data.height,16,"www.prechin.net");
	LCD_ShowString(10,50,tftlcd_data.width,tftlcd_data.height,16,"K_UP: Font Update");

	while(1)
	{
FONT:		
		LCD_ShowFont16Char(10, 180, "普中科技");
		LCD_ShowFont12Char(10, 201, "PRECHIN");
		LCD_ShowFont12Char(10, 222, "www.prechin.net");    
		LCD_ShowFont12Char(10, 243, "欢迎使用普中STM32F1开发板");
		LCD_ShowFont12Char(10, 260, "按下K_UP按键进行字库更新....");
		
		while(1)
		{
			key=KEY_Scan(0);
			if(key==KEY_UP)
			{
				FontUpdate(GUI_UPDATE_ALL);
				LCD_Clear(WHITE);
				goto FONT;
			}
				
			i++;
			if(i%20==0)
			{
				led1=!led1;
			}
			delay_ms(10);
		}
		
	}
	
}
