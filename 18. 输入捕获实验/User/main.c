

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "input.h"


/*******************************************************************************
* 函 数 名         : main
* 函数功能		   : 主函数
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
int main()
{
	u8 i=0;
	u32 indata=0;
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	TIM5_CH1_Input_Init(0xffff,71);  //以1M频率计数
	
	while(1)
	{
		if(TIM5_CH1_CAPTURE_STA&0x80) //成功捕获
		{
			indata=TIM5_CH1_CAPTURE_STA&0x3f;
			indata*=0xffff; //溢出次数乘以一次的计数次数时间 us
			indata+=TIM5_CH1_CAPTURE_VAL;//加上高电平捕获的时间
			printf("高电平持续时间：%d us\r\n",indata); //总的高电平时间
			TIM5_CH1_CAPTURE_STA=0; //开始下一次捕获
		}
		
		i++;
		if(i%20==0)
		{
			led1=!led1;
		}
		delay_ms(10);
	}
}
