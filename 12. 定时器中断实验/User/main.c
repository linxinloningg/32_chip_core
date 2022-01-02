

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "time.h"

int main()
{
	u8 i;
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	TIM4_Init(1000,36000-1);  //定时500ms
	
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
