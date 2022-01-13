#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "tftlcd.h"
#include "key.h"
#include "touch.h"

//清空屏幕并在右上角显示"RST"
void Load_Drow_Dialog(void)
{
	LCD_Clear(WHITE);//清屏   
 	FRONT_COLOR=BLUE;//设置字体为蓝色 
	LCD_ShowString(tftlcd_data.width-24,0,200,16,16,"RST");//显示清屏区域
  	FRONT_COLOR=RED;//设置画笔蓝色 
}

//画一个大点(2*2的点)		   
//x,y:坐标
//color:颜色
void TP_Draw_Big_Point(u16 x,u16 y,u16 color)
{	    
	FRONT_COLOR=color;
	LCD_DrawPoint(x,y);//中心点 
	LCD_DrawPoint(x+1,y);
	LCD_DrawPoint(x,y+1);
	LCD_DrawPoint(x+1,y+1);	 	  	
}
////////////////////////////////////////////////////////////////////////////////
//电容触摸屏专有部分
//画水平线
//x0,y0:坐标
//len:线长度
//color:颜色
void gui_draw_hline(u16 x0,u16 y0,u16 len,u16 color)
{
	if(len==0)return;
	LCD_Fill(x0,y0,x0+len-1,y0,color);	
}
//画实心圆
//x0,y0:坐标
//r:半径
//color:颜色
void gui_fill_circle(u16 x0,u16 y0,u16 r,u16 color)
{											  
	u32 i;
	u32 imax = ((u32)r*707)/1000+1;
	u32 sqmax = (u32)r*(u32)r+(u32)r/2;
	u32 x=r;
	gui_draw_hline(x0-r,y0,2*r,color);
	for (i=1;i<=imax;i++) 
	{
		if ((i*i+x*x)>sqmax)// draw lines from outside  
		{
 			if (x>imax) 
			{
				gui_draw_hline (x0-i+1,y0+x,2*(i-1),color);
				gui_draw_hline (x0-i+1,y0-x,2*(i-1),color);
			}
			x--;
		}
		// draw lines from inside (center)  
		gui_draw_hline(x0-x,y0+i,2*x,color);
		gui_draw_hline(x0-x,y0-i,2*x,color);
	}
}  
//两个数之差的绝对值 
//x1,x2：需取差值的两个数
//返回值：|x1-x2|
u16 my_abs(u16 x1,u16 x2)
{			 
	if(x1>x2)return x1-x2;
	else return x2-x1;
}  
//画一条粗线
//(x1,y1),(x2,y2):线条的起始坐标
//size：线条的粗细程度
//color：线条的颜色
void lcd_draw_bline(u16 x1, u16 y1, u16 x2, u16 y2,u8 size,u16 color)
{
	u16 t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance; 
	int incx,incy,uRow,uCol; 
	if(x1<size|| x2<size||y1<size|| y2<size)return; 
	delta_x=x2-x1; //计算坐标增量 
	delta_y=y2-y1; 
	uRow=x1; 
	uCol=y1; 
	if(delta_x>0)incx=1; //设置单步方向 
	else if(delta_x==0)incx=0;//垂直线 
	else {incx=-1;delta_x=-delta_x;} 
	if(delta_y>0)incy=1; 
	else if(delta_y==0)incy=0;//水平线 
	else{incy=-1;delta_y=-delta_y;} 
	if( delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴 
	else distance=delta_y; 
	for(t=0;t<=distance+1;t++ )//画线输出 
	{  
		gui_fill_circle(uRow,uCol,size,color);//画点 
		xerr+=delta_x ; 
		yerr+=delta_y ; 
		if(xerr>distance) 
		{ 
			xerr-=distance; 
			uRow+=incx; 
		} 
		if(yerr>distance) 
		{ 
			yerr-=distance; 
			uCol+=incy; 
		} 
	}  
} 

//5个触控点的颜色(电容触摸屏用)												 
const u16 POINT_COLOR_TBL[5]={RED,GREEN,BLUE,BROWN,GRED}; 

//电容触摸屏测试函数
void ctp_test(void)
{
	u8 t=0;
	u8 i=0;	  	    
 	u16 lastpos[5][2];		//最后一次的数据 
	while(1)
	{
		TOUCH_Scan();
		for(t=0;t<5;t++)
		{	
			if(TouchData.tpsta&(1<<t))
			{
				if(TouchData.lcdx[t]<tftlcd_data.width&&TouchData.lcdy[t]<tftlcd_data.height)
				{
					if(lastpos[t][0]==0XFFFF)
					{
						lastpos[t][0] = TouchData.lcdx[t];
						lastpos[t][1] = TouchData.lcdy[t];
					}
					lcd_draw_bline(lastpos[t][0],lastpos[t][1],TouchData.lcdx[t],TouchData.lcdy[t],2,POINT_COLOR_TBL[t]);//画线
					lastpos[t][0]=TouchData.lcdx[t];
					lastpos[t][1]=TouchData.lcdy[t];
					if(TouchData.lcdx[t]>(tftlcd_data.width-24)&&TouchData.lcdy[t]<20)
					{
						Load_Drow_Dialog();//清除
					}
				}
			}
			else lastpos[t][0]=0XFFFF;
		}
		delay_ms(5);i++;
		if(i%20==0)led1=!led1;
	}	
}

//电阻触摸屏测试函数
void rtp_test(void)
{
	u8 key;
	u8 i=0;	  
	while(1)
	{
	 	key=KEY_Scan(0);
		if(TOUCH_Scan()==0)
		{
			if(TouchData.lcdx[0]<tftlcd_data.width&&TouchData.lcdy[0]<tftlcd_data.height)
			{	
				if(TouchData.lcdx[0]>(tftlcd_data.width-24)&&TouchData.lcdy[0]<16)Load_Drow_Dialog();//清除
				else TP_Draw_Big_Point(TouchData.lcdx[0],TouchData.lcdy[0],RED);		//画图	  			   
			}
			else delay_ms(10);	//没有按键按下的时候
		}			
		 	    
		if(key==KEY_UP)	//KEY_UP按下,则执行校准程序
		{
			TOUCH_Adjust(); //屏幕校准  
			Load_Drow_Dialog();
		}
		i++;
		if(i%20==0)led1=!led1;
	}
}

int main()
{	
	u8 i=0;
	
	SysTick_Init(168);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	TFTLCD_Init();			//LCD初始化
	KEY_Init();
	TOUCH_Init();
	FRONT_COLOR=RED;//设置字体为红色 
	LCD_ShowString(10,10,200,16,16,"PRECHIN STM32F4");	
	LCD_ShowString(10,30,200,16,16,"TOUCH Test");	
	LCD_ShowString(10,50,200,16,16,"www.prechin.cn");
	LCD_ShowString(10,70,200,16,16,"Press K_UP to Adjust");
	delay_ms(1500);
 	Load_Drow_Dialog();
	while(1)
	{
		#ifdef TFTLCD_HX8352C
			rtp_test();
		#endif
		
		#ifdef TFTLCD_ILI9488
			ctp_test();
		#endif
		
		#ifdef TFTLCD_NT35510
			ctp_test();
		#endif
		
	}
}


