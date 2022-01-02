

#include "system.h"
#include "SysTick.h"
#include "led.h"



int main()
{
	SysTick_Init(72);
	LED_Init();
	while(1)
	{
		led1=0;
		led2=1;
		delay_ms(500);  //精确延时500ms
		led1=1;
		led2=0;
		delay_ms(500);  //精确延时500ms
	}
}
