#include "system.h"
#include "SysTick.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "tftlcd.h"
#include "24cxx.h"
#include "touch.h"
#include "malloc.h" 
#include "flash.h"
#include "sram.h"
#include "time.h"




int main()
{	
	u8 i;
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	TFTLCD_Init();			//LCD初始化
	TOUCH_Init();
	FSMC_SRAM_Init();  

	my_mem_init(SRAMIN);		//初始化内部内存池
	my_mem_init(SRAMEX);		//初始化外部SRAM内存池
	
	LCD_ShowString(10,10,200,200,16,"STemWin Transplant Demo");
	
 	while(1)
	{
		i++;
		if(i%20==0)
		{
			led1=!led1;
		}
		delay_ms(10);
	}
}




