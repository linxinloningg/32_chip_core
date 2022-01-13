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
#include "vs10xx.h"
#include "mp3player.h"




int main()
{
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	TFTLCD_Init();			//LCD初始化
	KEY_Init();
	EN25QXX_Init();				//初始化EN25Q128	  
	VS_Init();	  				//初始化VS1053
	my_mem_init(SRAMIN);		//初始化内部内存池
	FATFS_Init();							//为fatfs相关变量申请内存
	f_mount(fs[0],"0:",1); 		//挂载SD卡 
 	f_mount(fs[1],"1:",1); 		//挂载FLASH.
	
	FRONT_COLOR=RED;//设置字体为红色 
	while(SD_Init()!=0)
	{	
		LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"SD Card Error!");
	}
					 
	LCD_ShowFont16Char(10, 10, "普中科技");
    LCD_ShowFont12Char(10, 30, "www.prechin.cn");
    LCD_ShowFont12Char(10, 50, "音乐播放器实验");
    LCD_ShowFont12Char(10, 70, "K_UP：音量+");
    LCD_ShowFont12Char(10, 90, "K_DOWN：音量-");
    LCD_ShowFont12Char(10, 110, "K_RIGHT：下一曲");
	LCD_ShowFont12Char(10, 130, "K_LEFT：上一曲");
	
	while(1)
	{
  		led2=0; 	  
		LCD_ShowFont12Char(10,170,"存储器测试...");
		printf("Ram Test:0X%04X\r\n",VS_Ram_Test());//打印RAM测试结果	    
		LCD_ShowFont12Char(10,170,"正弦波测试..."); 	 	 
 		VS_Sine_Test();	
		LCD_ShowFont12Char(10,170,"《MP3音乐播放实验》");		
		led2=1;
		mp3_play();
	} 	   	
	
}
