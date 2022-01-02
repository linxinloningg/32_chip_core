

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "key.h"
#include "iwdg.h"


/*******************************************************************************
* 函 数 名         : main
* 函数功能		   : 主函数
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
int main()
{
	u8 i=0; 
		
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	
	KEY_Init();
	IWDG_Init(4,800); //只要在1280ms内进行喂狗就不会复位系统
	
	led2=1;
	printf("复位系统\r\n");
	
	while(1)
	{
		if(KEY_Scan(0)==KEY_UP)
		{
			IWDG_FeedDog();//喂狗
			led2=0;
			printf("喂狗\r\n");
		}
		
		i++;
		if(i%20==0)
		{
			led1=!led1;
			
		}
		delay_ms(10);
	}
}
