

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "oled.h"
#include "picture.h"


//OLDE 0.9寸4线SPI模块与普中STM32开发板接线方式
//GND  接电源地
//VCC  接5V或3.3v电源
//D0   接PD6（SCL）
//D1   接PD7（SDA）
//RES  接PD4
//DC   接PD5
//CS   接PD3

int main()
{
	u8 i=0;
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	OLED_Init(); //OLED初始化
	
	OLED_DrawBMP(0,0,128,8,(u8 *)pic1);	  //如果要正着显示，需要将取模方式修改重新取模
	delay_ms(1500);
	OLED_Clear();

	OLED_ShowString(0,0,"PRECHIN",16);

	OLED_ShowFontHZ(16*0,20,0,16,1);	 //深圳普中科技
	OLED_ShowFontHZ(16*1,20,1,16,1);
	OLED_ShowFontHZ(16*2,20,2,16,1);
	OLED_ShowFontHZ(16*3,20,3,16,1);
	OLED_ShowFontHZ(16*4,20,4,16,1);
	OLED_ShowFontHZ(16*5,20,5,16,1);

	OLED_ShowString(0,40,"www.prechin.cn",16);
	OLED_Refresh_Gram();  //刷新GRAM数组

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
