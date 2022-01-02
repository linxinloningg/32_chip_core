

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "adc.h"
#include "time.h"


float vol[5];
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
	ADCx_Init();
	TIM4_Init(5000,7200-1);  //定时100ms
	
	
	while(1)
	{
		i++;
		if(i%20==0)
		{
			led1=!led1;
		}
		
		if(i%50==0)
		{
//			vol[0]=(float)ADC_ConvertValue[0]*(3.3/4096);
//			printf("检测vol[0]电压值为：%.2fV\r\n",vol[0]);
//			vol[1]=(float)ADC_ConvertValue[1]*(3.3/4096);
//			printf("检测vol[1]电压值为：%.2fV\r\n",vol[1]);
//			vol[2]=(float)ADC_ConvertValue[2]*(3.3/4096);
//			printf("检测vol[2]电压值为：%.2fV\r\n",vol[2]);
//			vol[3]=(float)ADC_ConvertValue[3]*(3.3/4096);
//			printf("检测vol[3]电压值为：%.2fV\r\n",vol[3]);
//			vol[4]=(float)ADC_ConvertValue[4]*(3.3/4096);
//			printf("检测vol[4]电压值为：%.2fV\r\n",vol[4]);
		}
		delay_ms(10);	
	}
}
