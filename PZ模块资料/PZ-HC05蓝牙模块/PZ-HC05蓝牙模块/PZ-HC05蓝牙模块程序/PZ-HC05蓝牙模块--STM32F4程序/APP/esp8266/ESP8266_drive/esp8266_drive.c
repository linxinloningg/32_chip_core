#include "esp8266_drive.h"
#include "SysTick.h"
#include <stdio.h>  
#include <string.h>  
#include <stdbool.h>
#include "sta_tcpclent_test.h"
#include "esp8266_public.h"



struct STRUCT_USART_Fram ESP8266_Fram_Record_Struct = { 0 };




void USART3_Init(u32 bound)
{

  	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,ENABLE);	
	
	
	//串口3对应引脚复用映射
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource10,GPIO_AF_USART3); //GPIOB10复用为USART3
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource11,GPIO_AF_USART3); //GPIOB11复用为USART3
	
	//USART3端口配置
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_11 ; //GPIOB10与GPIOB11
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
	GPIO_Init(GPIOB,&GPIO_InitStructure); 
	

   //USART3 初始化设置
	USART_InitStructure.USART_BaudRate = bound;//波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
	USART_Init(USART3, &USART_InitStructure); //初始化串口3	
  
	//Usart3 NVIC 配置
  	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;//抢占优先级0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		//子优先级0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器

  	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);//开启串口接受和总线空闲中断
  	USART_ITConfig(USART3, USART_IT_IDLE, ENABLE);//开启串口接受和总线空闲中断
  	
	USART_Cmd(USART3, ENABLE);                    //使能串口3 	
}

void USART3_IRQHandler( void )
{	
	u8 ucCh;
	
	if(USART_GetITStatus( USART3, USART_IT_RXNE ) != RESET )
	{
		ucCh  = USART_ReceiveData( USART3 );
		
		if(ESP8266_Fram_Record_Struct .InfBit .FramLength < ( RX_BUF_MAX_LEN - 1 ) ) 
		{
			//预留1个字节写结束符
			ESP8266_Fram_Record_Struct .Data_RX_BUF[ ESP8266_Fram_Record_Struct .InfBit .FramLength ++ ]  = ucCh;	
		}                      
	}
	 	 
	if( USART_GetITStatus( USART3, USART_IT_IDLE ) == SET )                                         //数据帧接收完毕
	{
    	ESP8266_Fram_Record_Struct .InfBit .FramFinishFlag = 1;
		
		ucCh = USART_ReceiveData( USART3 );                                                              //由软件序列清除中断标志位(先读USART_SR，然后读USART_DR)
	
  	}	

}

void ESP8266_Init(u32 bound)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_AHB1PeriphClockCmd(ESP8266_RST_Pin_Periph_Clock|ESP8266_CH_PD_Pin_Periph_Clock, ENABLE);

	GPIO_InitStructure.GPIO_Pin =ESP8266_RST_Pin; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
	GPIO_Init(ESP8266_RST_Pin_Port, &GPIO_InitStructure);

   	GPIO_InitStructure.GPIO_Pin = ESP8266_CH_PD_Pin;			   
	GPIO_Init(ESP8266_CH_PD_Pin_Port, &GPIO_InitStructure);
	
	USART3_Init(bound);
	
	ESP8266_RST_Pin_SetH;
	ESP8266_CH_PD_Pin_SetL;	
}



//对ESP8266模块发送AT指令
// cmd：待发送的指令
// ack1，ack2：期待的响应，为NULL表不需响应，两者为或逻辑关系
// time：等待响应的时间
//返回1：发送成功 0：失败
bool ESP8266_Send_AT_Cmd(char *cmd,char *ack1,char *ack2,u32 time)
{
	ESP8266_Fram_Record_Struct .InfBit .FramLength = 0;	//从新开始接收新的数据包
	ESP8266_USART("%s\r\n", cmd);
	if(ack1==0&&ack2==0)	 //不需要接收数据
	{
		return true;
	}
	delay_ms(time);	  //延时time时间

	ESP8266_Fram_Record_Struct.Data_RX_BUF[ESP8266_Fram_Record_Struct.InfBit.FramLength ] = '\0';

	PC_USART("%s", ESP8266_Fram_Record_Struct .Data_RX_BUF);

	if(ack1!=0&&ack2!=0)
	{
		return ( ( bool ) strstr ( ESP8266_Fram_Record_Struct .Data_RX_BUF, ack1 ) || 
						 ( bool ) strstr ( ESP8266_Fram_Record_Struct .Data_RX_BUF, ack2 ) );
	}
	else if( ack1 != 0 )
		return ( ( bool ) strstr ( ESP8266_Fram_Record_Struct .Data_RX_BUF, ack1 ) );
	
	else
		return ( ( bool ) strstr ( ESP8266_Fram_Record_Struct .Data_RX_BUF, ack2 ) );
			
}


//重启WF-ESP8266模块
void ESP8266_Rst(void)
{
	ESP8266_RST_Pin_SetL;
	delay_ms(500); 
	ESP8266_RST_Pin_SetH;
}


//对ESP8266模块进行AT测试启动
void ESP8266_AT_Test(void)
{
	char count=0;
	
	ESP8266_RST_Pin_SetH;
	delay_ms(1000);	
	while(count < 10)
	{
		if(ESP8266_Send_AT_Cmd("AT","OK",NULL,500)) 
			return;
		ESP8266_Rst();
		++ count;
	}
}


//选择ESP8266模块的工作模式
// enumMode：工作模式
//返回1：选择成功 0：选择失败
bool ESP8266_Net_Mode_Choose(ENUM_Net_ModeTypeDef enumMode)
{
	switch ( enumMode )
	{
		case STA:
			return ESP8266_Send_AT_Cmd ( "AT+CWMODE=1", "OK", "no change", 2500 ); 
		
	  	case AP:
		  	return ESP8266_Send_AT_Cmd ( "AT+CWMODE=2", "OK", "no change", 2500 ); 
		
		case STA_AP:
		  	return ESP8266_Send_AT_Cmd ( "AT+CWMODE=3", "OK", "no change", 2500 ); 
		
	  	default:
		  return false;
	}		
}


//ESP8266模块连接外部WiFi
//pSSID：WiFi名称字符串
//pPassWord：WiFi密码字符串
//返回1：连接成功 0：连接失败
bool ESP8266_JoinAP( char * pSSID, char * pPassWord )
{
	char cCmd [120];

	sprintf ( cCmd, "AT+CWJAP=\"%s\",\"%s\"", pSSID, pPassWord );
	
	return ESP8266_Send_AT_Cmd( cCmd, "OK", NULL, 5000 );
	
}

/*
 * 函数名：ESP8266_CIPAP
 * 描述  ：设置模块的 AP IP
 * 输入  ：pApIp，模块的 AP IP
 * 返回  : 1，设置成功
 *         0，设置失败
 * 调用  ：被外部调用
 */
uint8_t ESP8266_CIPAP ( char * pApIp )
{
	char cCmd [ 30 ];	
	
	sprintf ( cCmd, "AT+CIPAP=\"%s\"", pApIp );
	
  if ( ESP8266_Send_AT_Cmd ( cCmd, "OK", 0, 5000 ) )
		return 1;
	else 
		return 0;
	
}

/*
 * 函数名：ESP8266_BuildAP
 * 描述  ：WF-ESP8266模块创建WiFi热点
 * 输入  ：pSSID，WiFi名称字符串
 *       ：pPassWord，WiFi密码字符串
 *       ：enunPsdMode，WiFi加密方式代号字符串
 * 返回  : 1，创建成功
 *         0，创建失败
 * 调用  ：被外部调用
 */
bool ESP8266_BuildAP ( char * pSSID, char * pPassWord, ENUM_AP_PsdMode_TypeDef enunPsdMode )
{
	char cCmd [120];

	sprintf ( cCmd, "AT+CWSAP=\"%s\",\"%s\",1,%d", pSSID, pPassWord, enunPsdMode );
	
	return ESP8266_Send_AT_Cmd ( cCmd, "OK", 0, 1000 );
	
}


//ESP8266模块启动多连接
//enumEnUnvarnishTx：配置是否多连接
//返回1：配置成功 0：配置失败
bool ESP8266_Enable_MultipleId (FunctionalState enumEnUnvarnishTx )
{
	char cStr [20];
	
	sprintf ( cStr, "AT+CIPMUX=%d", ( enumEnUnvarnishTx ? 1 : 0 ) );
	
	return ESP8266_Send_AT_Cmd ( cStr, "OK", 0, 500 );
	
}


//ESP8266模块连接外部服务器
//enumE：网络协议
//ip：服务器IP字符串
//ComNum：服务器端口字符串
//id：模块连接服务器的ID
//返回1：连接成功 0：连接失败
bool ESP8266_Link_Server(ENUM_NetPro_TypeDef enumE, char * ip, char * ComNum, ENUM_ID_NO_TypeDef id)
{
	char cStr [100] = { 0 }, cCmd [120];

  	switch (  enumE )
  	{
		case enumTCP:
		  sprintf ( cStr, "\"%s\",\"%s\",%s", "TCP", ip, ComNum );
		  break;
		
		case enumUDP:
		  sprintf ( cStr, "\"%s\",\"%s\",%s", "UDP", ip, ComNum );
		  break;
		
		default:
			break;
  	}

  	if ( id < 5 )
    	sprintf ( cCmd, "AT+CIPSTART=%d,%s", id, cStr);

  	else
	  	sprintf ( cCmd, "AT+CIPSTART=%s", cStr );

	return ESP8266_Send_AT_Cmd ( cCmd, "OK", "ALREAY CONNECT", 4000 );
	
}


/*
 * 函数名：ESP8266_StartOrShutServer
 * 描述  ：WF-ESP8266模块开启或关闭服务器模式
 * 输入  ：enumMode，开启/关闭
 *       ：pPortNum，服务器端口号字符串
 *       ：pTimeOver，服务器超时时间字符串，单位：秒
 * 返回  : 1，操作成功
 *         0，操作失败
 * 调用  ：被外部调用
 */
bool ESP8266_StartOrShutServer ( FunctionalState enumMode, char * pPortNum, char * pTimeOver )
{
	char cCmd1 [120], cCmd2 [120];

	if ( enumMode )
	{
		sprintf ( cCmd1, "AT+CIPSERVER=%d,%s", 1, pPortNum );
		
		sprintf ( cCmd2, "AT+CIPSTO=%s", pTimeOver );

		return ( ESP8266_Send_AT_Cmd ( cCmd1, "OK", 0, 500 ) &&
						 ESP8266_Send_AT_Cmd ( cCmd2, "OK", 0, 500 ) );
	}
	
	else
	{
		sprintf ( cCmd1, "AT+CIPSERVER=%d,%s", 0, pPortNum );

		return ESP8266_Send_AT_Cmd ( cCmd1, "OK", 0, 500 );
	}
	
}

/*
 * 函数名：ESP8266_Inquire_ApIp
 * 描述  ：获取 F-ESP8266 的 AP IP
 * 输入  ：pApIp，存放 AP IP 的数组的首地址
 *         ucArrayLength，存放 AP IP 的数组的长度
 * 返回  : 0，获取失败
 *         1，获取成功
 * 调用  ：被外部调用
 */
uint8_t ESP8266_Inquire_ApIp ( char * pApIp, uint8_t ucArrayLength )
{
	char uc;
	
	char * pCh;
	
	
	ESP8266_Send_AT_Cmd ( "AT+CIFSR", "OK", 0, 500 );
	
	pCh = strstr ( ESP8266_Fram_Record_Struct .Data_RX_BUF, "APIP,\"" );
	
	if ( pCh )
		pCh += 6;
	
	else
		return 0;
	
	for ( uc = 0; uc < ucArrayLength; uc ++ )
	{
		pApIp [ uc ] = * ( pCh + uc);
		
		if ( pApIp [ uc ] == '\"' )
		{
			pApIp [ uc ] = '\0';
			break;
		}
		
	}
	
	return 1;
	
}


//配置ESP8266模块进入透传发送
//返回1：配置成功 0：配置失败
bool ESP8266_UnvarnishSend ( void )
{
	if (!ESP8266_Send_AT_Cmd ( "AT+CIPMODE=1", "OK", 0, 500 ))
		return false;
	
	return 
	  	ESP8266_Send_AT_Cmd( "AT+CIPSEND", "OK", ">", 500 );
	
}


//ESP8266模块发送字符串
//enumEnUnvarnishTx：声明是否已使能了透传模式
//pStr：要发送的字符串
//ulStrLength：要发送的字符串的字节数
//ucId：哪个ID发送的字符串
//返回1：发送成功 0：发送失败
bool ESP8266_SendString(FunctionalState enumEnUnvarnishTx, char * pStr, u32 ulStrLength, ENUM_ID_NO_TypeDef ucId )
{
	char cStr [20];
	bool bRet = false;
	
		
	if ( enumEnUnvarnishTx )
	{
		ESP8266_USART ( "%s", pStr );
		
		bRet = true;
		
	}

	else
	{
		if ( ucId < 5 )
			sprintf ( cStr, "AT+CIPSEND=%d,%d", ucId, ulStrLength + 2 );

		else
			sprintf ( cStr, "AT+CIPSEND=%d", ulStrLength + 2 );
		
		ESP8266_Send_AT_Cmd ( cStr, "> ", 0, 1000 );

		bRet = ESP8266_Send_AT_Cmd ( pStr, "SEND OK", 0, 1000 );
  }
	
	return bRet;

}


//ESP8266模块退出透传模式
void ESP8266_ExitUnvarnishSend ( void )
{
	delay_ms(1000);
	ESP8266_USART( "+++" );
	delay_ms( 500 ); 	
}


//ESP8266 的连接状态，较适合单端口时使用
//返回0：获取状态失败
//返回2：获得ip
//返回3：建立连接 
//返回4：失去连接 
u8 ESP8266_Get_LinkStatus ( void )
{
	if (ESP8266_Send_AT_Cmd( "AT+CIPSTATUS", "OK", 0, 500 ) )
	{
		if ( strstr ( ESP8266_Fram_Record_Struct .Data_RX_BUF, "STATUS:2\r\n" ) )
			return 2;
		
		else if ( strstr ( ESP8266_Fram_Record_Struct .Data_RX_BUF, "STATUS:3\r\n" ) )
			return 3;
		
		else if ( strstr ( ESP8266_Fram_Record_Struct .Data_RX_BUF, "STATUS:4\r\n" ) )
			return 4;		

	}
	
	return 0;
	
}

