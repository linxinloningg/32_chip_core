

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "key.h"
#include "dac.h"


/*******************************************************************************
* 函 数 名         : main
* 函数功能		   : 主函数
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
int main()
{
	u8 i=0;
	u8 key;
	int dac_value=0;
	u16 dacval;
	float dac_vol;
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	KEY_Init();
	DAC1_Init();
	
	while(1)
	{
		key=KEY_Scan(0);
		if(key==KEY_UP)
		{
			dac_value+=400;
			if(dac_value>=4000)
			{
				dac_value=4095;
			}
			DAC_SetChannel1Data(DAC_Align_12b_R,dac_value);
		}
		else if(key==KEY_DOWN)
		{
			dac_value-=400;	
			if(dac_value<=0)
			{
				dac_value=0;
			}
			DAC_SetChannel1Data(DAC_Align_12b_R,dac_value);			
		}
		
		i++;
		if(i%20==0)
		{
			led1=!led1;
		}
		
		if(i%50==0)
		{
			dacval=DAC_GetDataOutputValue(DAC_Channel_1);
			dac_vol=(float)dacval*(3.3/4096);
			printf("输出DAC电压值为：%.2fV\r\n",dac_vol);
		}
		delay_ms(10);	
	}
}
