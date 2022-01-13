#include "system.h"
#include "SysTick.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "tftlcd.h"
#include "24cxx.h"
#include "touch.h"
#include "malloc.h" 
#include "flash.h"
#include "sram.h"
#include "time.h"
#include "GUI.h"
#include "GUIDemo.h"


void Touch_Test(void)
{
	GUI_PID_STATE TouchState;
	int xPhys;
	int yPhys;
	GUI_Init();
	GUI_SetFont(&GUI_Font20_ASCII);
	GUI_CURSOR_Show();
	GUI_CURSOR_Select(&GUI_CursorCrossL);
	GUI_SetBkColor(GUI_WHITE);
	GUI_SetColor(GUI_BLACK);
	GUI_Clear();
	GUI_DispString("Measurement of\nA/D converter values");
	while(1)
	{
		GUI_TOUCH_GetState(&TouchState); // Get the touch position in pixel
		xPhys = GUI_TOUCH_GetxPhys(); // Get the A/D mesurement result in x
		yPhys = GUI_TOUCH_GetyPhys(); // Get the A/D mesurement result in y
		GUI_SetColor(GUI_BLUE);
		GUI_DispStringAt("Analog input:\n", 0, 40);
		GUI_GotoY(GUI_GetDispPosY() + 2);
		GUI_DispString("x:");
		GUI_DispDec(xPhys, 4);
		GUI_DispString(", y:");
		GUI_DispDec(yPhys, 4);
		GUI_SetColor(GUI_RED);
		GUI_GotoY(GUI_GetDispPosY() + 4);
		GUI_DispString("\nPosition:\n");
		GUI_GotoY(GUI_GetDispPosY() + 2);
		GUI_DispString("x:");
		GUI_DispDec(TouchState.x,4);
		GUI_DispString(", y:");
		GUI_DispDec(TouchState.y,4);
		delay_ms(50);

	}
}


int main()
{	
	u8 i;
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	TFTLCD_Init();			//LCD初始化
	TOUCH_Init();
	FSMC_SRAM_Init(); 

	my_mem_init(SRAMIN);		//初始化内部内存池
	my_mem_init(SRAMEX);		//初始化外部SRAM内存池
	
	TIM3_Init(999,71);	//1KHZ 定时器1ms 
	TIM6_Init(999,719);	//10ms中断
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC,ENABLE);//使能CRC时钟，否则STemWin不能使用
	WM_SetCreateFlags(WM_CF_MEMDEV);
	GUI_Init();
	GUIDEMO_Main();
	//Touch_Test();
 	while(1);
}




