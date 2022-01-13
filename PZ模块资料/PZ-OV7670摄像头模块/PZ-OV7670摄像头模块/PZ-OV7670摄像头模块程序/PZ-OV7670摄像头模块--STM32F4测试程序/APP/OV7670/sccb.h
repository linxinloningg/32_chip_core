#ifndef _sccb_H
#define _sccb_H


#include "system.h"


//IO操作函数	 
#define SCCB_SCL    		PDout(6) 	//SCL
#define SCCB_SDA    		PDout(7) 	//SDA	 

#define SCCB_READ_SDA    	PDin(7)  		//输入SDA    
#define SCCB_ID   			0X42  			//OV7670的ID

///////////////////////////////////////////
void SCCB_Init(void);
void SCCB_Start(void);
void SCCB_Stop(void);
void SCCB_No_Ack(void);
u8 SCCB_WR_Byte(u8 dat);
u8 SCCB_RD_Byte(void);
u8 SCCB_WR_Reg(u8 reg,u8 data);
u8 SCCB_RD_Reg(u8 reg);



#endif

