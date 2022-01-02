

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "key.h"


int main()
{
	u8 key,i;
	SysTick_Init(72);
	LED_Init();
	KEY_Init();
	
	while(1)
	{
		key=KEY_Scan(0);   //扫描按键
		switch(key)
		{
			case KEY_UP: led2=0;break;      //按下K_UP按键    点亮D2指示灯
			case KEY_DOWN: led2=1;break;    //按下K_DOWN按键  熄灭D2指示灯
			case KEY_LEFT: led3=1;break;    //按下K_LEFT按键  点亮D3指示灯
			case KEY_RIGHT: led3=0;break;   //按下K_RIGHT按键 熄灭D3指示灯
		}
		i++;
		if(i%20==0)
		{
			led1=!led1;      //LED1状态取反
		}
		delay_ms(10);	
	}
}
