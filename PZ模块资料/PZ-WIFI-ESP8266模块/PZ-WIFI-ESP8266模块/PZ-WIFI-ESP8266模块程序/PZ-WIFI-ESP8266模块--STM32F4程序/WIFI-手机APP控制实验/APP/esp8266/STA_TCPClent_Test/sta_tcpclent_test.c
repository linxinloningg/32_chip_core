#include "sta_tcpclent_test.h"
#include "SysTick.h"
#include "usart.h"
#include "esp8266_drive.h"
#include "ds18b20.h"
#include <string.h> 
#include "led.h"
#include "beep.h"



void ESP8266_STA_TCPClient_Test(void)
{

	uint8_t ucId, ucLen;
	char str[100]={0};
	char cCh;
	char * pCh, * pCh1;
	float temp=0.0;
	
	printf ( "\r\n正在配置ESP8266请耐心等待...\r\n" );

	ESP8266_CH_PD_Pin_SetH;

	ESP8266_AT_Test();
	
	ESP8266_Net_Mode_Choose(AP);
	
	while (!ESP8266_CIPAP(User_ESP8266_TCPServer_IP ));
	
	while (!ESP8266_BuildAP(User_ESP8266_BulitApSsid, User_ESP8266_BulitApPwd, User_ESP8266_BulitApEcn));	
	
	ESP8266_Enable_MultipleId ( ENABLE );
	
	while(!ESP8266_StartOrShutServer(ENABLE, User_ESP8266_TCPServer_PORT, User_ESP8266_TCPServer_OverTime ));
	
	ESP8266_Inquire_ApIp ( str, 20 );
	
	printf ( "\r\n配置ESP8266 OK！\r\n" );
	printf ( "\r\n本模块WIFI为%s，密码开放\r\nAP IP 为：%s，开启的端口为：%s\r\n手机网络助手连接该 IP 和端口，最多可连接5个客户端\r\n",
           User_ESP8266_BulitApSsid, str, User_ESP8266_TCPServer_PORT );
	
	ESP8266_Fram_Record_Struct .InfBit .FramLength = 0;
	ESP8266_Fram_Record_Struct .InfBit .FramFinishFlag = 0;
	
	while ( 1 )
	{		
		if(ESP8266_Fram_Record_Struct .InfBit .FramFinishFlag )
		{
			USART_ITConfig ( USART3, USART_IT_RXNE, DISABLE ); //禁用串口接收中断
			ESP8266_Fram_Record_Struct .Data_RX_BUF [ ESP8266_Fram_Record_Struct .InfBit .FramLength ]  = '\0';
			
			printf ( "\r\n%s\r\n", ESP8266_Fram_Record_Struct .Data_RX_BUF );//
			if ( ( pCh = strstr ( ESP8266_Fram_Record_Struct .Data_RX_BUF, "+IPD,0,6:LED_" ) ) != 0 )   
			{
				cCh = * ( pCh + 13 );
				
				switch ( cCh )
				{
					case '1':                //LED1控制
						cCh = * ( pCh + 14 );
					  switch ( cCh )
					  {
							case '0':     //关闭LED1
								led1=1;
								break;
							case '1':     //打开LED1
								led1=0;
								break;
							default :
								break;
						}
						break;
						
					case '2':                   //LED2控制
						cCh = * ( pCh + 14 );
					  switch ( cCh )
					  {
							case '0':
								led2=1;
								break;
							case '1':
								led2=0;
								break;
							default :
								break;
						}
						break;
					
					default :
						break;	
				}
				
			}
			
			else if ( ( pCh = strstr ( ESP8266_Fram_Record_Struct .Data_RX_BUF, "+IPD,0,8:BUZZER_" ) ) != 0 ) 
			{
				cCh = * ( pCh + 16 );
				
				switch ( cCh )
				{
					case '0':
						beep=1;
						break;
					case '1':
						beep=0;
						break;
					default:
						break;
				}
				
			}
				
			temp=DS18B20_GetTemperture();
			if ( ( pCh = strstr ( ESP8266_Fram_Record_Struct .Data_RX_BUF, "+IPD,0,6:Temp" ) ) != 0 ) 
			{
				sprintf ( str, "%0.2f",temp );
				ucId = * ( pCh + strlen ( "+IPD," ) ) - '0';
				ESP8266_SendString ( DISABLE, str, strlen ( str ), ( ENUM_ID_NO_TypeDef ) ucId );
				printf("\r\nstr=%s\r\n",str);
			}
			
			ESP8266_Fram_Record_Struct .InfBit .FramLength = 0;
			ESP8266_Fram_Record_Struct .InfBit .FramFinishFlag = 0;	
			
			USART_ITConfig ( USART3, USART_IT_RXNE, ENABLE ); //使能串口接收中断
			
		}
	}
		
}



