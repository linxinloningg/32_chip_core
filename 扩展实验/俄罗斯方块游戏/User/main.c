#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "tftlcd.h"
#include "time.h"
#include "key.h"
#include "tetris.h"



int main()
{
	u8 i;
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	TFTLCD_Init();			//LCD初始化
	KEY_Init();
	LCD_Clear(GREEN);
	LCD_ShowPictureEx(0, 0, 240, 320); 
	
	TIM4_Init(10,7199);
	while(key!= KEY_UP) //等待按键K_UP按下
	{
		FRONT_COLOR=RED;
		LCD_ShowString(20,10,tftlcd_data.width,tftlcd_data.height,16,"Press K_UP key to Enter...");
		delay_ms(200);
		LCD_ShowString(20,10,tftlcd_data.width,tftlcd_data.height,16,"                          ");
		delay_ms(200);
	}
	TIM_Cmd(TIM4, DISABLE);  //失能TIMx
	
	score_buf[0]=Game.score/100000+0x30;
	score_buf[1]=Game.score%100000/10000+0x30;
	score_buf[2]=Game.score%100000%10000/1000+0x30;
	score_buf[3]=Game.score%100000%10000%1000/100+0x30;
	score_buf[4]=Game.score%100000%10000%1000%100/10+0x30;
	score_buf[5]=Game.score%100000%10000%1000%100%10+0x30;
	score_buf[6]='\0';

	level_buf[0]=Game.level/100+0x30;
	level_buf[1]=Game.level%100/10+0x30;
	level_buf[2]=Game.level%100%10+0x30;
	level_buf[3]='\0';
	Show_TetrisFace();
	Start_Game();
	TIM3_Init(5000,7199);
	
	while(1)
	{
		key=KEY_Scan(0);
		switch(key)
		{
			case KEY_LEFT:MoveLeft();break;
			case KEY_RIGHT:MoveRight();break;
			case KEY_DOWN:DownFast();break;
			case KEY_UP:Transform();break;
		//	case RESET:ResetGame();break;	
			default :
							break;
		}
		i++;
		if(i%20==0)
		{
			led1=!led1;
		}
		delay_ms(10);
	}
}
