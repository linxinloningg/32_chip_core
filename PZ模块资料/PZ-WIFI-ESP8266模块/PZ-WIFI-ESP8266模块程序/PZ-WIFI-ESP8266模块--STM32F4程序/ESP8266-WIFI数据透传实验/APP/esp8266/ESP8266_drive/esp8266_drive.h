#ifndef _esp8266_drive_H
#define _esp8266_drive_H

#include "system.h"
#include <stdio.h>
#include <stdbool.h>



#if defined ( __CC_ARM   )
#pragma anon_unions
#endif

//ESP8266数据类型定义
typedef enum
{
	STA,
  	AP,
  	STA_AP  
}ENUM_Net_ModeTypeDef;


typedef enum{
	 enumTCP,
	 enumUDP,
} ENUM_NetPro_TypeDef;

typedef enum{
	Multiple_ID_0 = 0,
	Multiple_ID_1 = 1,
	Multiple_ID_2 = 2,
	Multiple_ID_3 = 3,
	Multiple_ID_4 = 4,
	Single_ID_0 = 5,
} ENUM_ID_NO_TypeDef;

#define ESP8266_RST_Pin          GPIO_Pin_6
#define ESP8266_RST_Pin_Port     GPIOF
#define ESP8266_RST_Pin_Periph_Clock  RCC_AHB1Periph_GPIOF

#define ESP8266_CH_PD_Pin     GPIO_Pin_7
#define ESP8266_CH_PD_Pin_Port     GPIOC
#define ESP8266_CH_PD_Pin_Periph_Clock  RCC_AHB1Periph_GPIOC


#define ESP8266_RST_Pin_SetH     GPIO_SetBits(ESP8266_RST_Pin_Port,ESP8266_RST_Pin)
#define ESP8266_RST_Pin_SetL     GPIO_ResetBits(ESP8266_RST_Pin_Port,ESP8266_RST_Pin)


#define ESP8266_CH_PD_Pin_SetH     GPIO_SetBits(ESP8266_CH_PD_Pin_Port,ESP8266_CH_PD_Pin)
#define ESP8266_CH_PD_Pin_SetL     GPIO_ResetBits(ESP8266_CH_PD_Pin_Port,ESP8266_CH_PD_Pin)


#define ESP8266_USART(fmt, ...)	 USART_printf (USART3, fmt, ##__VA_ARGS__)
//#define ESP8266_USART(fmt, ...)	 USART_printf (USART2, fmt, ##__VA_ARGS__)
#define PC_USART(fmt, ...)	 printf (fmt, ##__VA_ARGS__)



#define RX_BUF_MAX_LEN 1024		  //最大接收缓存字节数
extern struct STRUCT_USART_Fram	  //定义一个全局串口数据帧的处理结构体
{
	char Data_RX_BUF[RX_BUF_MAX_LEN];
	union 
	{
    	__IO u16 InfAll;
    	struct 
		{
		  	__IO u16 FramLength       :15;                               // 14:0 
		  	__IO u16 FramFinishFlag   :1;                                // 15 
	  	}InfBit;
  	}; 
}ESP8266_Fram_Record_Struct;





//函数的声明
void ESP8266_Init(u32 bound);
void ESP8266_AT_Test(void);
bool ESP8266_Send_AT_Cmd(char *cmd,char *ack1,char *ack2,u32 time);
void ESP8266_Rst(void);
bool ESP8266_Net_Mode_Choose(ENUM_Net_ModeTypeDef enumMode);
bool ESP8266_JoinAP( char * pSSID, char * pPassWord );
bool ESP8266_Enable_MultipleId ( FunctionalState enumEnUnvarnishTx );
bool ESP8266_Link_Server(ENUM_NetPro_TypeDef enumE, char * ip, char * ComNum, ENUM_ID_NO_TypeDef id);
bool ESP8266_SendString(FunctionalState enumEnUnvarnishTx, char * pStr, u32 ulStrLength, ENUM_ID_NO_TypeDef ucId );
bool ESP8266_UnvarnishSend ( void );
void ESP8266_ExitUnvarnishSend ( void );
u8 ESP8266_Get_LinkStatus ( void );

#endif
