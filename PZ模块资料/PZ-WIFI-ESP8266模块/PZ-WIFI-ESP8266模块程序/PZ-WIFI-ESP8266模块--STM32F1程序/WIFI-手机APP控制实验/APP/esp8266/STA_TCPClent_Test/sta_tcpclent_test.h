#ifndef _sta_tcpclent_test_H
#define _sta_tcpclent_test_H


#include "system.h"


#define User_ESP8266_BulitApSsid	  "PRECHIN"	  //要建立的热点的名称
#define User_ESP8266_BulitApEcn	  	  OPEN            //要建立的热点的加密方式
#define User_ESP8266_BulitApPwd  	  "prechin"      //要建立的热点的密钥


#define User_ESP8266_TCPServer_IP	  "192.168.1.119"	  //服务器开启的IP地址
#define User_ESP8266_TCPServer_PORT	  "8080"	  //服务器开启的端口


#define User_ESP8266_TCPServer_OverTime	  "1800"	  //服务器超时时间（单位：秒）


void ESP8266_STA_TCPClient_Test(void);
void TIM4_Init(u16 per,u16 psc);



#endif
