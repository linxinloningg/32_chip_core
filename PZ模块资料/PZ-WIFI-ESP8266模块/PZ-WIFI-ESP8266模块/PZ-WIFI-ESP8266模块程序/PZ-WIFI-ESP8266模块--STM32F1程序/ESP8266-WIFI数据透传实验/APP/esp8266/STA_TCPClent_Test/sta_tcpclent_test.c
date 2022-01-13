#include "sta_tcpclent_test.h"
#include "SysTick.h"
#include "usart.h"
#include "esp8266_drive.h"


volatile u8 TcpClosedFlag = 0;


void ESP8266_STA_TCPClient_Test(void)
{
	u8 res;
	
	char str[100]={0};
	
	printf ( "\r\n正在配置ESP8266请耐心等待...\r\n" );

	ESP8266_CH_PD_Pin_SetH;

	ESP8266_AT_Test();
	ESP8266_Net_Mode_Choose(STA);
	while(!ESP8266_JoinAP(User_ESP8266_SSID, User_ESP8266_PWD));
	ESP8266_Enable_MultipleId ( DISABLE );
	while(!ESP8266_Link_Server(enumTCP, User_ESP8266_TCPServer_IP, User_ESP8266_TCPServer_PORT, Single_ID_0));
	
	while(!ESP8266_UnvarnishSend());
	printf ( "\r\n配置ESP8266 OK！\r\n" );
	
	while ( 1 )
	{		
		sprintf (str,"www.prechin.cn\r\n" );
		ESP8266_SendString ( ENABLE, str, 0, Single_ID_0 );
		delay_ms(1000);
		if(TcpClosedFlag) //检测是否失去连接
		{
			ESP8266_ExitUnvarnishSend(); //退出透传模式
			do
			{
				res = ESP8266_Get_LinkStatus();     //获取连接状态
			} 	
			while(!res);
			
			if(res == 4)                     //确认失去连接后重连
			{
				printf ( "\r\n请稍等，正在重连热点和服务器...\r\n" );
				
				while (!ESP8266_JoinAP(User_ESP8266_SSID, User_ESP8266_PWD ) );
				
				while (!ESP8266_Link_Server(enumTCP, User_ESP8266_TCPServer_IP, User_ESP8266_TCPServer_PORT, Single_ID_0 ) );
				
				printf ( "\r\n重连热点和服务器成功\r\n" );
			} 
			while(!ESP8266_UnvarnishSend());					
		}
	}
		
}



