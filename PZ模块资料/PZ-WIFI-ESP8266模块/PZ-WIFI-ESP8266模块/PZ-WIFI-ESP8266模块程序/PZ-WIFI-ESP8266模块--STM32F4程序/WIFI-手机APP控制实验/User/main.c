#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "beep.h"
#include "usart.h"
#include "ds18b20.h"
#include "esp8266_drive.h"
#include "sta_tcpclent_test.h"


int main()
{	
	u8 i;
	float temp;
	SysTick_Init(168);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	USART1_Init(115200);
	LED_Init();
	BEEP_Init();
	DS18B20_Init();
	
//	while(1)
//	{
//		i++;
//		if(i%50==0)
//		{
//			temp=DS18B20_GetTemperture();
//			printf("temp=%f\r\n",temp);
//		}
//		delay_ms(10);
//		
//	}
	printf("普中科技ESP8266 WIFI模块手机APP控制测试\r\n");
	ESP8266_Init(115200);
	
	ESP8266_STA_TCPClient_Test();
	
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


