

#include "system.h"
#include "SysTick.h"
#include "usart.h"
#include "tftlcd.h"
#include "touch.h"
u16 penColor = BLUE;
u16 BackgroundColor= BLACK;
void display_init()  //初始化显示
{
	FRONT_COLOR=BLUE;
	LCD_Fill(250,0,320,50,WHITE);
	LCD_ShowString(270,15,tftlcd_data.width,tftlcd_data.height,16,"Erase");
	LCD_Fill(0,0,49,50, BLUE);
  LCD_Fill(50,0,99,50,BRED);
  LCD_Fill(100,0,149,50,YELLOW);
	LCD_Fill(150,0,199,50,GREEN);
  LCD_Fill(200,0,249,50, RED);
}
void Colorchiose(void)
{

              if(TouchData.lcdx>=0&&TouchData.lcdx<=49)
                {
                    penColor =BLUE;
                }
                else if(TouchData.lcdx>=50&&TouchData.lcdx<=99)
                {
                    penColor =BRED;
                }
                
                else if(TouchData.lcdx>=100&&TouchData.lcdx<=149)
                {
                    penColor =YELLOW;
                }
                else if(TouchData.lcdx>=150&&TouchData.lcdx<=199)
                {
                   penColor =GREEN;
                }
                else if(TouchData.lcdx>=200&&TouchData.lcdx<=249)
                {
                    penColor = RED;

                }
                else if(TouchData.lcdx>=250&&TouchData.lcdx<=299)
                {
                    penColor =GREEN;
                }       	
}
void Rst_judgment(void)
{
	if ((TouchData.lcdx>=250&&TouchData.lcdx<=320)) 
			{
				LCD_Clear(BackgroundColor);
				display_init();
			}         
}
int main()
{	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	USART1_Init(9600);
	TFTLCD_Init();			//LCD初始化
	LCD_Clear(BackgroundColor);
	TOUCH_Init();
	display_init();
	
	while(1)
	{
		if(TOUCH_Scan() == 0)
        {   
					if(TouchData.lcdy<60)   // = TFT_YMAX - 18
            {
            /* 选择画笔的颜色 */
            Colorchiose();
						/* 清屏 */
           Rst_judgment();
            }
            else   //画点
            {
LCD_Fill(TouchData.lcdx-1, TouchData.lcdy-1, TouchData.lcdx+2,TouchData.lcdy+2, penColor);
            }    
        }
	}
	
}
