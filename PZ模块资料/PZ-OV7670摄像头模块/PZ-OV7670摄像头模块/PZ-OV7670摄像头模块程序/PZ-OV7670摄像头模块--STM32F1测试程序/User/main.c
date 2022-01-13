

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "tftlcd.h"
#include "key.h"
#include "malloc.h" 
#include "sd.h"
#include "flash.h"
#include "ff.h" 
#include "fatfs_app.h"
#include "key.h"
#include "font_show.h"
#include "exti.h"
#include "time.h" 
#include "string.h"		
#include "math.h"	 
#include "ov7670.h"



extern u8 ov_sta;	//在exit.c里面定义
extern u8 ov_frame;	//在time.c里面定义



//更新LCD显示
void camera_refresh(void)
{
	u32 j;
	u16 i;
 	u16 color;
	u16 temp;
	if(ov_sta)//有帧中断更新？
	{
		LCD_Display_Dir(1);
		//LCD_Set_Window((tftlcd_data.width-320)/2,(tftlcd_data.height-240)/2,320,240-1);//将显示区域设置到屏幕中央
		LCD_Set_Window(0,(tftlcd_data.height-240)/2,320-1,240-1);//将显示区域设置到屏幕中央
		
		OV7670_RRST=0;				//开始复位读指针 
		OV7670_RCK_L;
		OV7670_RCK_H;
		OV7670_RCK_L;
		OV7670_RRST=1;				//复位读指针结束 
		OV7670_RCK_H;
		/*for(i=0;i<240;i++)   //此种方式可以兼容任何彩屏,但是速度很慢
		{
			for(j=0;j<320;j++)
			{
				OV7670_RCK_L;
				color=GPIOF->IDR&0XFF;	//读数据
				OV7670_RCK_H; 
				color<<=8;  
				OV7670_RCK_L;
				color|=GPIOF->IDR&0XFF;	//读数据
				OV7670_RCK_H; 
				LCD_DrawFRONT_COLOR(j,i,color);
			}
		}*/
		for(j=0;j<76800;j++)   //此种方式需清楚TFT内部显示方向控制寄存器值 速度较快
		{
			OV7670_RCK_L;
			color=GPIOF->IDR&0XFF;	//读数据
			OV7670_RCK_H; 
			color<<=8;  
			OV7670_RCK_L;
			color|=GPIOF->IDR&0XFF;	//读数据
			OV7670_RCK_H; 
			LCD_WriteData_Color(color); 
			//printf("%x  ",color);
			//if(j%20==0)printf("\r\n");
			//delay_us(50);
		}   							  
 		ov_sta=0;					//清零帧中断标记
		ov_frame++; 
		LCD_Display_Dir(0);
	} 
}

const u8*LMODE_TBL[5]={"Auto","Sunny","Cloudy","Office","Home"};
const u8*EFFECTS_TBL[7]={"Normal","Negative","B&W","Redish","Greenish","Bluish","Antique"};	//7种特效 


int main()
{
	u8 i=0;
	u8 key;
	u8 lightmode=0,saturation=2,brightness=2,contrast=2;
	u8 effect=0;
	u8 sbuf[15];
	u8 count;
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	TFTLCD_Init();			//LCD初始化
	KEY_Init();
	EN25QXX_Init();				//初始化EN25Q128	  
	my_mem_init(SRAMIN);		//初始化内部内存池
	
	FRONT_COLOR=RED;//设置字体为红色 
	
//	while(SD_Init()!=0)
//	{	
//		LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"SD Card Error!");
//	}
//	FATFS_Init();							//为fatfs相关变量申请内存				 
//  	f_mount(fs[0],"0:",1); 					//挂载SD卡 
// 	f_mount(fs[1],"1:",1); 				//挂载FLASH.
		
	LCD_ShowFont12Char(10, 10, "普中科技");
	LCD_ShowFont12Char(10, 30, "www.prechin.net");    
	LCD_ShowFont12Char(10, 50, "摄像头应用--OV7670");
	i=OV7670_Init();
	printf("i=%d\n",i);
	while(OV7670_Init())//初始化OV7670
	{
		LCD_ShowString(10,80,tftlcd_data.width,tftlcd_data.height,16,"OV7670 Error!");
		delay_ms(200);
		LCD_Fill(10,80,239,206,WHITE);
		delay_ms(200);
	}
 	LCD_ShowString(10,80,tftlcd_data.width,tftlcd_data.height,16,"OV7670 OK!     ");
	delay_ms(1500);	 
	OV7670_Light_Mode(0);
	OV7670_Color_Saturation(2);
	OV7670_Brightness(2);
	OV7670_Contrast(2);
 	OV7670_Special_Effects(0);
		
	TIM4_Init(10000,7199);			//10Khz计数频率,1秒钟中断									  
	EXTI7_Init();			
	OV7670_Window_Set(12,176,240,320);	//设置窗口	
  	OV7670_CS=0;	
	LCD_Clear(BLACK);
	while(1)
	{
		key=KEY_Scan(0);
		if(key)count=20;
		switch(key)
		{
			case KEY_UP:           //灯光模式设置
				lightmode++;
				if(lightmode>4)lightmode=0;
				OV7670_Light_Mode(lightmode);
				sprintf((char*)sbuf,"%s",LMODE_TBL[lightmode]);
				break;
			case KEY_DOWN:         //饱和度
				saturation++;
				if(saturation>4)saturation=0;
				OV7670_Color_Saturation(saturation);
				sprintf((char*)sbuf,"Saturation:%d",(char)saturation-2);
				break;	
			case KEY_LEFT:        //亮度
				brightness++;
				if(brightness>4)brightness=0;
				OV7670_Brightness(brightness);
				sprintf((char*)sbuf,"Brightness:%d",(char)brightness-2);
				break;
			case KEY_RIGHT:     //对比度
				contrast++;
				if(contrast>4)contrast=0;
				OV7670_Contrast(contrast);
				sprintf((char*)sbuf,"Contrast:%d",(char)contrast-2);
				break;
		}
		if(count)
		{
			count--;
			LCD_ShowString((tftlcd_data.width-240)/2+30,(tftlcd_data.height-320)/2+60,200,16,16,sbuf);
		}
		camera_refresh();//更新显示
		i++;
		if(i%20==0)
		{
			led1=!led1;
		}
		//delay_ms(5);	
	}
	
}
