

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "ds18b20.h"


int main()
{
	u8 i=0;
	float temper;
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	while(DS18B20_Init())
	{
		printf("DS18B20检测失败，请插好!\r\n");
		delay_ms(500);
	}
	printf("DS18B20检测成功!\r\n");
	
	while(1)
	{
		i++;
		if(i%20==0)
		{
			led1=!led1;
		}
		
		if(i%50==0)
		{
			temper=DS18B20_GetTemperture();
			if(temper<0)
			{
				printf("检测的温度为：-");
			}
			else
			{
				printf("检测的温度为： ");
			}
			printf("%.2f°C\r\n",temper);
		}
		delay_ms(10);
	}
}
