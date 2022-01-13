#ifndef _ctiic_H
#define _ctiic_H


#include "system.h"


//IO操作函数	 
#define CT_IIC_SCL    PBout(0) 			//SCL     
#define CT_IIC_SDA    PFout(11) 		//SDA	 
#define CT_READ_SDA   PFin(11)  			//输入SDA 
 

//IIC所有操作函数
void CT_IIC_Init(void);                	//初始化IIC的IO口				 
void CT_IIC_Start(void);				//发送IIC开始信号
void CT_IIC_Stop(void);	  				//发送IIC停止信号
void CT_IIC_Send_Byte(u8 txd);			//IIC发送一个字节
u8 CT_IIC_Read_Byte(unsigned char ack);	//IIC读取一个字节
u8 CT_IIC_Wait_Ack(void); 				//IIC等待ACK信号
void CT_IIC_Ack(void);					//IIC发送ACK信号
void CT_IIC_NAck(void);					//IIC不发送ACK信号






#endif
