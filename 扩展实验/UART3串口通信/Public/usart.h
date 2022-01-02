#ifndef __usart_H
#define __usart_H

#include "system.h" 
#include "stdio.h" 

//#define USART1_PRINTF
#define USART3_PRINTF

#define USART_REC_LEN  			200  	//定义最大接收字节数 200
#define EN_USART1_RX 			1		//使能（1）/禁止（0）串口1接收
	  	
extern u8  USART_RX_BUF[USART_REC_LEN]; //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 
extern u16 USART_RX_STA;         		//接收状态标记	

void USART1_Init(u32 bound);
void USART3_Init(u32 bound);


#endif


