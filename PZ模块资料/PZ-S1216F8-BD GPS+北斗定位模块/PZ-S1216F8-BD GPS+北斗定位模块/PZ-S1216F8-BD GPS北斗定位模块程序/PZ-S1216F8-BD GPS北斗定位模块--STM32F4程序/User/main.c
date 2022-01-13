#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "key.h" 
#include "usart.h"
#include "tftlcd.h"
#include "gps.h"
#include "usart3.h"			 	 
#include "string.h"



u8 USART1_TX_BUF[USART3_MAX_RECV_LEN]; 					//串口1,发送缓存区
nmea_msg gpsx; 											//GPS信息
__align(4) u8 dtbuf[50];   								//打印缓存器
const u8*fixmode_tbl[4]={"Fail","Fail"," 2D "," 3D "};	//fix mode字符串 
	  
//显示GPS定位信息 
void Gps_Msg_Show(void)
{
 	float tp;		   
	FRONT_COLOR=BLUE;  	 
	tp=gpsx.longitude;	   
	sprintf((char *)dtbuf,"Longitude:%.5f %1c   ",tp/=100000,gpsx.ewhemi);	//得到经度字符串
 	LCD_ShowString(10,120,200,16,16,dtbuf);	 	   
	tp=gpsx.latitude;	   
	sprintf((char *)dtbuf,"Latitude:%.5f %1c   ",tp/=100000,gpsx.nshemi);	//得到纬度字符串
 	LCD_ShowString(10,140,200,16,16,dtbuf);	 	 
	tp=gpsx.altitude;	   
 	sprintf((char *)dtbuf,"Altitude:%.1fm     ",tp/=10);	    			//得到高度字符串
 	LCD_ShowString(10,160,200,16,16,dtbuf);	 			   
	tp=gpsx.speed;	   
 	sprintf((char *)dtbuf,"Speed:%.3fkm/h     ",tp/=1000);		    		//得到速度字符串	 
 	LCD_ShowString(10,180,200,16,16,dtbuf);	 				    
	if(gpsx.fixmode<=3)														//定位状态
	{  
		sprintf((char *)dtbuf,"Fix Mode:%s",fixmode_tbl[gpsx.fixmode]);	
	  LCD_ShowString(10,200,200,16,16,dtbuf);			   
	}	 	   
	sprintf((char *)dtbuf,"GPS+BD Valid satellite:%02d",gpsx.posslnum);	 		//用于定位的GPS卫星数
 	LCD_ShowString(10,220,200,16,16,dtbuf);	    
	sprintf((char *)dtbuf,"GPS Visible satellite:%02d",gpsx.svnum%100);	 		//可见GPS卫星数
 	LCD_ShowString(10,240,200,16,16,dtbuf);
	
	sprintf((char *)dtbuf,"BD Visible satellite:%02d",gpsx.beidou_svnum%100);	 		//可见北斗卫星数
 	LCD_ShowString(10,260,200,16,16,dtbuf);
	
	sprintf((char *)dtbuf,"UTC Date:%04d/%02d/%02d   ",gpsx.utc.year,gpsx.utc.month,gpsx.utc.date);	//显示UTC日期
	LCD_ShowString(10,280,200,16,16,dtbuf);		    
	sprintf((char *)dtbuf,"UTC Time:%02d:%02d:%02d   ",gpsx.utc.hour,gpsx.utc.min,gpsx.utc.sec);	//显示UTC时间
  LCD_ShowString(10,300,200,16,16,dtbuf);		  	  
}



int main()
{
	u16 i,rxlen;
	u16 lenx;
	u8 key=0XFF;
	u8 upload=0; 
	
	SysTick_Init(168);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	KEY_Init();
	USART1_Init(9600);
	TFTLCD_Init();			//LCD初始化
	USART3_Init(38400);
	
	FRONT_COLOR=RED;
	LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"PRECHIN");
	LCD_ShowString(10,30,tftlcd_data.width,tftlcd_data.height,16,"www.prechin.com");
	LCD_ShowString(10,50,tftlcd_data.width,tftlcd_data.height,16,"S1216F8-BD GPS Test");
	LCD_ShowString(10,70,tftlcd_data.width,tftlcd_data.height,16,"K_UP:Upload NMEA Data SW");
	LCD_ShowString(10,90,tftlcd_data.width,tftlcd_data.height,16,"NMEA Data Upload:OFF");
	
	
	if(S1216F8BD_Cfg_Rate(5)!=0)	//设置定位信息更新速度为5Hz,顺便判断GPS模块是否在位. 
	{
		LCD_ShowString(10,120,200,16,16,"S1216F8-BD GPS Setting...");
		do
		{
			USART3_Init(9600);			//初始化串口3波特率为9600
			S1216F8BD_Cfg_Prt(3);			//重新设置模块的波特率为38400
			USART3_Init(38400);			//初始化串口3波特率为38400
			key=S1216F8BD_Cfg_Tp(100000);	//脉冲宽度为100ms
		}while(S1216F8BD_Cfg_Rate(5)!=0&&key!=0);//配置S1216F8BDF8-BD的更新速率为5Hz
		LCD_ShowString(10,120,200,16,16,"S1216F8-BD GPS Set Done!!");
		delay_ms(500);
		LCD_Fill(10,120,10+200,120+16,WHITE);//清除显示 
	}
	while(1) 
	{		
		delay_ms(1);
		if(USART3_RX_STA&0X8000)		//接收到一次数据了
		{
			rxlen=USART3_RX_STA&0X7FFF;	//得到数据长度
			for(i=0;i<rxlen;i++)USART1_TX_BUF[i]=USART3_RX_BUF[i];	   
 			USART3_RX_STA=0;		   	//启动下一次接收
			USART1_TX_BUF[i]=0;			//自动添加结束符
			GPS_Analysis(&gpsx,(u8*)USART1_TX_BUF);//分析字符串
			Gps_Msg_Show();				//显示信息	
			if(upload)printf("\r\n%s\r\n",USART1_TX_BUF);//发送接收到的数据到串口1
 		}
		key=KEY_Scan(0);
		if(key==KEY_UP)
		{
			upload=!upload;
			FRONT_COLOR=RED;
			if(upload)LCD_ShowString(10,90,200,16,16,"NMEA Data Upload:ON ");
			else LCD_ShowString(10,90,200,16,16,"NMEA Data Upload:OFF");
 		}
		if((lenx%500)==0)
			led1=!led1;
		lenx++;	
	}
}



