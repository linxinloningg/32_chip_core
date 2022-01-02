

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "key.h"
#include "24cxx.h"


int main()
{
	u8 i=0;
	u8 key;
	u8 k=0;
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	KEY_Init();
	AT24CXX_Init();
	while(AT24CXX_Check())  //检测AT24C02是否正常
	{
		printf("AT24C02检测不正常!\r\n");
		delay_ms(500);
	}
	printf("AT24C02检测正常!\r\n");
	
	while(1)
	{
		key=KEY_Scan(0);
		if(key==KEY_UP)
		{
			k++;
			if(k>255)
			{
				k=255;
			}
			AT24CXX_WriteOneByte(0,k);
			printf("写入的数据是：%d\r\n",k);
		}
		if(key==KEY_DOWN)
		{
			k=AT24CXX_ReadOneByte(0);
			printf("读取的数据是：%d\r\n",k);
		}
		i++;
		if(i%20==0)
		{
			led1=!led1;
		}
		
		delay_ms(10);
			
	}
}
