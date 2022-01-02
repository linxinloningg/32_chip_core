

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "adc.h"


/*******************************************************************************
* 函 数 名         : main
* 函数功能		   : 主函数
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
int main()
{
	u8 i=0;
	u16 value=0;
	float vol;
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	ADCx_Init();
	
	while(1)
	{
		i++;
		if(i%20==0)
		{
			led1=!led1;
		}
		
		if(i%50==0)
		{
			value=Get_ADC_Value(ADC_Channel_1,20);
			printf("检测AD值为：%d\r\n",value);
			vol=(float)value*(3.3/4096);
			printf("检测电压值为：%.2fV\r\n",vol);
		}
		delay_ms(10);	
	}
}
