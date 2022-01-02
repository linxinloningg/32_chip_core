

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "smg.h"


u8 smgduan[16]={0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07,
             0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71};//0~F 数码管段选数据

int main()
{
	u8 i=0;
	SysTick_Init(72);
	LED_Init();
	SMG_Init();
	
	while(1)
	{
		for(i=0;i<16;i++)
		{
			GPIO_Write(SMG_PORT,(u16)(~smgduan[i]));
			delay_ms(1000);	
		}	  
	}
}
