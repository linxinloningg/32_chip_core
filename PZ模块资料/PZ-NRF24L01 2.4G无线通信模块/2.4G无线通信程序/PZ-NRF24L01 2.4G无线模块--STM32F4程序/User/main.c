/* 下载程序后，首先要按下按键K_UP或者K_DOWN，按键K_UP是接收，K_DOWN是发送，两块开发板
	只能一个作为发送一个作为接收，否则两个都为接收或者发送将进入死循环。接收的时候
	指示灯闪烁  NRF24L01的最大SPI时钟为10Mhz因此在设定SPI时钟的时候要低于10M*/

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "tftlcd.h"
#include "key.h"
#include "nrf24l01.h"

void data_pros()	//数据处理函数
{
	u8 key;
	static u8 mode=2;  //模式选择
	u8 rx_buf[33]="www.prechin.cn";
	static u16 t=0;
	while(1)		//等待按键按下进行选择发送还是接收
	{
		key=KEY_Scan(0);	
		if(key==KEY_UP)	   //接收模式
		{
			mode=0;
			LCD_ShowString(10,140,tftlcd_data.width,tftlcd_data.height,16,"RX_Mode");
			LCD_ShowString(10,160,tftlcd_data.width,tftlcd_data.height,16,"Received Data:");
			LCD_ShowString(120,160,tftlcd_data.width,tftlcd_data.height,16,"                ");
			break;
		}
		if(key==KEY_DOWN)	 //发送模式
		{
			mode=1;
			LCD_ShowString(10,140,tftlcd_data.width,tftlcd_data.height,16,"TX_Mode");
			LCD_ShowString(10,160,tftlcd_data.width,tftlcd_data.height,16,"Send Data:    ");
			LCD_ShowString(120,160,tftlcd_data.width,tftlcd_data.height,16,"              ");
			break;
		}	
	}
	
	if(mode==0)		//接收模式
	{	
		NRF24L01_RX_Mode();	
		while(1)
		{
			if(NRF24L01_RxPacket(rx_buf)==0) //接收到数据显示
			{
				rx_buf[32]='\0';
				LCD_ShowString(120,160,tftlcd_data.width,tftlcd_data.height,16,rx_buf);
				break;			
			}
			else
			{
				delay_ms(1);
			}
			t++;
			if(t==1000)
			{
				t=0;
				led2=~led2; //一秒钟改变一次状态
			}	
		}	
	}
	if(mode==1)		 //发送模式
	{
				
		NRF24L01_TX_Mode();
		while(1)
		{
			if(NRF24L01_TxPacket(rx_buf)==TX_OK)
			{
				LCD_ShowString(120,160,tftlcd_data.width,tftlcd_data.height,16,rx_buf);
				break;	
			}
			else
			{
				LCD_ShowString(120,160,tftlcd_data.width,tftlcd_data.height,16,"Send Data Failed  ");
		
			}	
		}	
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
	NRF24L01_Init();
	
	
	FRONT_COLOR=BLACK;
	LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"PRECHIN STM32F4");
	LCD_ShowString(10,30,tftlcd_data.width,tftlcd_data.height,16,"www.prechin.net");
	LCD_ShowString(10,50,tftlcd_data.width,tftlcd_data.height,16,"NRF24L01 Test");
	LCD_ShowString(10,70,tftlcd_data.width,tftlcd_data.height,16,"K_UP:RX_Mode  K_DOWN:TX_Mode");
	FRONT_COLOR=RED;
	
	while(NRF24L01_Check())	 //检测NRF24L01是否存在
	{
		LCD_ShowString(140,50,tftlcd_data.width,tftlcd_data.height,16,"Error   ");			
	}
	LCD_ShowString(140,50,tftlcd_data.width,tftlcd_data.height,16,"Success");
	
	while(1)
	{
		data_pros(); 
		i++;
		if(i%20==0)
		{
			led1=!led1;
		}
		
		delay_ms(10);
			
	}
}


