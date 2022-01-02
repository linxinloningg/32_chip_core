#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "tftlcd.h"
#include "rtc.h"
#include "math.h"


#define PI 3.1415926 
void get_circle(int x,int y,int r,int col)
{
	int xc=0;
	int yc,p;
	yc=r;
	p=3-(r<<1);	
	while(xc <= yc)
	{
		LCD_DrawFRONT_COLOR(x+xc,y+yc,col);
		LCD_DrawFRONT_COLOR(x+xc,y-yc,col);	
		LCD_DrawFRONT_COLOR(x-xc,y+yc,col);
		LCD_DrawFRONT_COLOR(x-xc,y-yc,col);
		LCD_DrawFRONT_COLOR(x+yc,y+xc,col);	
		LCD_DrawFRONT_COLOR(x+yc,y-xc,col);
		LCD_DrawFRONT_COLOR(x-yc,y+xc,col);
		LCD_DrawFRONT_COLOR(x-yc,y-xc,col);
		if(p<0)
		{
			p += (xc++ << 2) + 6;	
		}
		else
			p += ((xc++ - yc--)<<2) + 10;
	}
}
void draw_circle()	 //画圆
{
	get_circle(100,200,100,YELLOW);
	get_circle(100,200,99,YELLOW);
	get_circle(100,200,98,YELLOW);
	get_circle(100,200,97,YELLOW);
	get_circle(100,200,5,YELLOW);			
}
void draw_dotline()  //画格点
{
	u8 i;
	u8 rome[][3]={"12","1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11" } ; //表盘数字 
	int x1,y1,x2,y2,x3,y3;
	for(i=0;i<60;i++)
	{
		x1 = (int)(100 + (sin(i * PI / 30) * 92)); 
		y1 = (int)(100 - (cos(i * PI / 30) * 92)); 
		x2 = (int)(100 + (sin(i * PI / 30) * 97)); 
		y2 = (int)(100 - (cos(i * PI / 30) * 97)); 
		FRONT_COLOR=RED;
		LCD_DrawLine(x1,y1+100,x2,y2+100);

		if(i%5==0)
		{
			x1 = (int)(100 + (sin(i * PI / 30) * 85)); 
			y1 = (int)(100 - (cos(i * PI / 30) * 85)); 
			x2 = (int)(100 + (sin(i * PI / 30) * 97)); 
			y2 = (int)(100 - (cos(i * PI / 30) * 97)); 
			LCD_DrawLine(x1,y1+100,x2,y2+100);	

			x3 = (int)(92 + (sin((i ) * PI / 30) * 80)); 
			y3 = (int)(92 - (cos((i ) * PI / 30) * 80));
			FRONT_COLOR=YELLOW;
			LCD_ShowString(x3,y3+100,tftlcd_data.width,tftlcd_data.height,16,rome[i/5]);
			
		}
		
	}		
}
void draw_hand(int hhour,int mmin,int ssec)  //画指针
{
	int xhour, yhour, xminute, yminute, xsecond, ysecond; //表心坐标系指针坐标
	xhour = (int)(60 * sin( hhour * PI / 6 + mmin * PI / 360 + ssec * PI / 1800)); 
	yhour = (int)(60 * cos( hhour * PI / 6 + mmin * PI / 360 + ssec * PI / 1800)); 
	xminute = (int)(90 * sin( mmin * PI / 30 + ssec * PI / 1800)); 
	yminute = (int)(90 * cos( mmin * PI / 30 + ssec * PI / 1800)); 
	xsecond = (int)(100 * sin( ssec * PI / 30)); 
	ysecond = (int)(100 * cos( ssec * PI / 30)); 

	FRONT_COLOR=RED;
	LCD_DrawLine(100 + xhour, 200 - yhour, 100 -xhour / 6, 200 + yhour / 6);
	FRONT_COLOR=BLUE;
	LCD_DrawLine(100 + xminute, 200 - yminute, 100 -xminute / 4, 200 + yminute / 4);
	FRONT_COLOR=GREEN;
	LCD_DrawLine(100 + xsecond, 200 - ysecond, 100 -xsecond / 3, 200 + ysecond / 3);
	
}
void draw_hand_clear(int hhour,int mmin,int ssec)  //擦指针
{
	int xhour, yhour, xminute, yminute, xsecond, ysecond; //表心坐标系指针坐标
	xhour = (int)(60 * sin( hhour * PI / 6 + mmin * PI / 360 + ssec * PI / 1800)); 
	yhour = (int)(60 * cos( hhour * PI / 6 + mmin * PI / 360 + ssec * PI / 1800)); 
	xminute = (int)(90 * sin( mmin * PI / 30 + ssec * PI / 1800)); 
	yminute = (int)(90 * cos( mmin * PI / 30 + ssec * PI / 1800)); 
	xsecond = (int)(100 * sin( ssec * PI / 30)); 
	ysecond = (int)(100 * cos( ssec * PI / 30)); 

	FRONT_COLOR=BLACK;
	LCD_DrawLine(100 + xhour, 200 - yhour, 100 -xhour / 6, 200 + yhour / 6);
	LCD_DrawLine(100 + xminute, 200 - yminute, 100 -xminute / 4, 200 + yminute / 4);
	LCD_DrawLine(100 + xsecond, 200 - ysecond, 100 -xsecond / 3, 200 + ysecond / 3);
	
}

int main()
{
	u8 buf[10];
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	TFTLCD_Init();			//LCD初始化
	
	LCD_Clear(BLACK);
	FRONT_COLOR=YELLOW;
	BACK_COLOR=BLACK;
	LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"This is a RTC text!");
	RTC_Init();
	draw_circle();
	draw_dotline();
	
	while(1)
	{
		if(timebz==1)
		{
			draw_hand_clear(calendar.hour,calendar.min,calendar.sec);
			timebz=0;
			RTC_Get();
			sprintf((char *)buf,"%.2d:%.2d:%.2d",calendar.hour,calendar.min,calendar.sec);
			draw_hand(calendar.hour,calendar.min,calendar.sec);

		}
		draw_circle();
		draw_dotline();
		LCD_ShowString(80,50,tftlcd_data.width,tftlcd_data.height,16,buf);		
	}			
}
