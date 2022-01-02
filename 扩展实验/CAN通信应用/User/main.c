

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "key.h"
#include "can.h"
#include "tftlcd.h"


int main()
{
	u8 i=0;
	u8 key,j=0;
	u8 mode=0; //CAN_Mode_LoopBack=1,	CAN_Mode_Normal=0
	u8 res;
	u8 tbuf[8],char_buf[9];
	u8 rbuf[8];

	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	KEY_Init();
	TFTLCD_Init();
	
	FRONT_COLOR=BLACK;
	LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"CAN Test! Buand:500Kbps");
	LCD_ShowString(10,90,tftlcd_data.width,tftlcd_data.height,16,"K_UP:MODE  K_DOWN:Send");	
	LCD_ShowString(10,120,tftlcd_data.width,tftlcd_data.height,16,"Mode:");
	LCD_ShowString(50,120,tftlcd_data.width,tftlcd_data.height,16,"CAN_Mode_Normal   ");
	LCD_ShowString(10,150,tftlcd_data.width,tftlcd_data.height,16,"Send Data:");
	LCD_ShowString(10,180,tftlcd_data.width,tftlcd_data.height,16,"Receive Data:");
	
	CAN_Mode_Init(CAN_SJW_1tq,CAN_BS2_6tq,CAN_BS1_7tq,6,CAN_Mode_Normal);//500Kbps波特率
	FRONT_COLOR=RED;
	while(1)
	{
		key=KEY_Scan(0);
		if(key==KEY_UP)  //模式切换
		{
			mode=!mode;
			CAN_Mode_Init(CAN_SJW_1tq,CAN_BS2_6tq,CAN_BS1_7tq,6,mode);
			LCD_ShowString(90,150,tftlcd_data.width,tftlcd_data.height,16,"                      ");
			LCD_ShowString(114,180,tftlcd_data.width,tftlcd_data.height,16,"                      ");
	
			if(mode==0)
			{
				printf("Normal Mode\r\n");
				LCD_ShowString(50,120,tftlcd_data.width,tftlcd_data.height,16,"CAN_Mode_Normal   ");
	
			}
			else
			{
				printf("LoopBack Mode\r\n");
				LCD_ShowString(50,120,tftlcd_data.width,tftlcd_data.height,16,"CAN_Mode_LoopBack");
	
			}
	
		}
		if(key==KEY_DOWN)  //发送数据
		{
			for(j=0;j<8;j++)
			{
				tbuf[j]=j;
				char_buf[j]=tbuf[j]+0x30;
			}
			char_buf[8]='\0';
			LCD_ShowString(90,150,tftlcd_data.width,tftlcd_data.height,16,char_buf);
			res=CAN_Send_Msg(tbuf,8);
			if(res)
			{
				printf("Send Failed!\r\n");
				LCD_ShowString(170,150,tftlcd_data.width,tftlcd_data.height,16,"Failed ");
			}
			else
			{
				printf("发送数据：%s\r\n",char_buf);
				LCD_ShowString(170,150,tftlcd_data.width,tftlcd_data.height,16,"Success");
			}
			
		}
		res=CAN_Receive_Msg(rbuf);
		if(res)
		{
			for(j=0;j<res;j++)
			{
				char_buf[j]=rbuf[j]+0x30;
			}
			char_buf[8]='\0';
			LCD_ShowString(114,180,tftlcd_data.width,tftlcd_data.height,16,char_buf);
			printf("接收数据：%s\r\n",char_buf);
		}
		
		i++;
		if(i%20==0)
		{
			led1=!led1;
		}
		
		delay_ms(10);
	}
}
