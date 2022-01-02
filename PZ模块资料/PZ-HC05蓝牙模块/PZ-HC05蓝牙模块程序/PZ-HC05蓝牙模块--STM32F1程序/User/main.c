

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "key.h"
#include "usart.h"
#include "tftlcd.h"
#include "hc05.h"
#include "usart2.h"			 	 
#include "string.h"

	

//显示HC05模块的主从状态
void HC05_Role_Show(void)
{
	if(HC05_Get_Role()==1)LCD_ShowString(10,140,200,16,16,"ROLE:Master");	//主机
	else LCD_ShowString(10,140,200,16,16,"ROLE:Slave ");			 		//从机
}

//显示HC05模块的连接状态
void HC05_Sta_Show(void)
{												 
	if(HC05_STATE)LCD_ShowString(110,140,120,16,16,"STA:Connected ");			//连接成功
	else LCD_ShowString(110,140,120,16,16,"STA:Disconnect");	 			//未连接				 
}


int main()
{
	u8 t=0;
	u8 key;
	u8 sendmask=0;
	u8 sendcnt=0;
	u8 sendbuf[20];	  
	u8 reclen=0; 
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	KEY_Init();
	USART1_Init(9600);
	TFTLCD_Init();			//LCD初始化
	
	
	FRONT_COLOR=RED;
	LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"PRECHIN");
	LCD_ShowString(10,30,tftlcd_data.width,tftlcd_data.height,16,"www.prechin.com");
	LCD_ShowString(10,50,tftlcd_data.width,tftlcd_data.height,16,"BT05 BlueTooth Test");
	delay_ms(1000);			//等待蓝牙模块上电稳定
	
	while(HC05_Init()) 		//初始化HC05模块  
	{
		LCD_ShowString(10,90,200,16,16,"HC05 Error!    "); 
		delay_ms(500);
		LCD_ShowString(10,90,200,16,16,"Please Check!!!"); 
		delay_ms(100);
	}	 										   	   
	LCD_ShowString(10,90,210,16,16,"K_UP:ROLE K_DOWN:SEND/STOP");  
	LCD_ShowString(10,110,200,16,16,"HC05 Standby!");  
  	LCD_ShowString(10,160,200,16,16,"Send:");	
	LCD_ShowString(10,180,200,16,16,"Receive:"); 
	
	FRONT_COLOR=BLUE;
	HC05_Role_Show();
	delay_ms(100);
	USART2_RX_STA=0;
 	while(1) 
	{		
		key=KEY_Scan(0);
		if(key==KEY_UP)						//切换模块主从设置
		{
   			key=HC05_Get_Role();
			if(key!=0XFF)
			{
				key=!key;  					//状态取反	   
				if(key==0)HC05_Set_Cmd("AT+ROLE=0");
				else HC05_Set_Cmd("AT+ROLE=1");
				HC05_Role_Show();
				HC05_Set_Cmd("AT+RESET");	//复位HC05模块
				delay_ms(200);
			}
		}
		else if(key==KEY_DOWN)
		{
			sendmask=!sendmask;				//发送/停止发送  	 
			if(sendmask==0)LCD_Fill(10+40,160,240,160+16,WHITE);//清除显示
		}
		else delay_ms(10);	   
		if(t==50)
		{
			if(sendmask)					//定时发送
			{
				sprintf((char*)sendbuf,"PREHICN HC05 %d\r\n",sendcnt);
	  			LCD_ShowString(10+40,160,200,16,16,sendbuf);	//显示发送数据	
				usart2_printf("PREHICN HC05 %d\r\n",sendcnt);	//发送到蓝牙模块
				sendcnt++;
				if(sendcnt>99)sendcnt=0;
			}
			HC05_Sta_Show();  	  
			t=0;
			led1=!led1; 	     
		}	  
		if(USART2_RX_STA&0X8000)			//接收到一次数据了
		{
			LCD_Fill(10,200,240,320,WHITE);	//清除显示
 			reclen=USART2_RX_STA&0X7FFF;	//得到数据长度
		  	USART2_RX_BUF[reclen]='\0';	 	//加入结束符
			printf("reclen=%d\r\n",reclen);
			printf("USART2_RX_BUF=%s\r\n",USART2_RX_BUF);
			if(reclen==10||reclen==11) 		//控制D2检测
			{
				if(strcmp((const char*)USART2_RX_BUF,"+LED2 ON\r\n")==0)led2=0;//打开LED2
				if(strcmp((const char*)USART2_RX_BUF,"+LED2 OFF\r\n")==0)led2=1;//关闭LED2
			}
 			LCD_ShowString(10,200,209,119,16,USART2_RX_BUF);//显示接收到的数据
 			USART2_RX_STA=0;	 
		}	 															     				   
		t++;	
	}
}
