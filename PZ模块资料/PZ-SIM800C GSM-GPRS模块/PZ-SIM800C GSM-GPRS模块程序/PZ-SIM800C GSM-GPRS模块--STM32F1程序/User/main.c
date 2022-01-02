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
#include "touch.h"
#include "usart2.h"
#include "sim800c.h"



int main()
{
	u8 key;
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	KEY_Init();
	USART1_Init(9600);
	TFTLCD_Init();	//LCD初始化
	EN25QXX_Init();	//初始化EN25Q128	 
	tp_dev.init();			//初始化触摸屏			 
	USART2_Init(115200);
	my_mem_init(SRAMIN);		//初始化内部内存池
	FATFS_Init();							//为fatfs相关变量申请内存				 
  	f_mount(fs[0],"0:",1); 					//挂载SD卡 
 	f_mount(fs[1],"1:",1); 				//挂载FLASH.
	key=KEY_Scan(1);
	if(key==KEY_UP)
	{
		LCD_Clear(WHITE);	                       //清屏
		TP_Adjust();  		                       //屏幕校准 
		TP_Save_Adjdata();	  
		LCD_Clear(WHITE);   	                   //清屏
	}
	EN25QXX_Init();	//初始化EN25Q128	
	FRONT_COLOR=RED;//设置字体为红色 
	sim800c_test();                                //GSM测试
	
}
