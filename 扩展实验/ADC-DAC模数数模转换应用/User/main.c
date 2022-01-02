

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "key.h"
#include "dac.h"
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
	float adc_vol;
	u16 dacval;
	float dac_vol;
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	KEY_Init();
	DAC1_Init();
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
			printf("ADC1_IN1检测AD值为：%d\r\n",value);
			adc_vol=(float)value*(3.3/4096);
			printf("ADC1_IN1检测电压值为：%.2fV\r\n",adc_vol);
			
			DAC_SetChannel1Data(DAC_Align_12b_R,value);
			
			dacval=DAC_GetDataOutputValue(DAC_Channel_1);
			dac_vol=(float)dacval*(3.3/4096);
			printf("输出DAC电压值为：%.2fV\r\n",dac_vol);
			
			printf("\r\n");
		}
		delay_ms(10);	
	}
}
