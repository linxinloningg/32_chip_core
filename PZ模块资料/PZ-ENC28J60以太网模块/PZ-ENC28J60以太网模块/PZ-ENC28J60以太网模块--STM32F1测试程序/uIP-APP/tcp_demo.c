#include "system.h"
#include "usart.h"	 		   
#include "uip.h"	    
#include "enc28j60.h"
#include "httpd.h"
#include "tcp_demo.h"


//TCP应用接口函数(UIP_APPCALL)
//完成TCP服务(包括server和client)和HTTP服务
void tcp_demo_appcall(void)
{	
  	
	switch(uip_conn->lport)//本地监听端口80和1200 
	{
		case HTONS(80):
			httpd_appcall(); 
			break;
		case HTONS(1200):
		    tcp_server_demo_appcall(); 
			break;
		default:						  
		    break;
	}		    
	switch(uip_conn->rport)	//远程连接1400端口
	{
	    case HTONS(1400):
			tcp_client_demo_appcall();
	       break;
	    default: 
	       break;
	}   
}
//打印日志用
void uip_log(char *m)
{			    
	printf("uIP log:%s\r\n",m);
}

























