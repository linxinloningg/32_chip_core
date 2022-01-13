

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "tftlcd.h"
#include "esp8266_drive.h"
#include "sta_tcpclent_test.h"
	

int main()
{
	u8 i;
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(115200);
	
	printf("普中科技ESP8266 WIFI模块STA TCPClent透传测试\r\n");
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
