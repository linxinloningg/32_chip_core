#ifndef _hc05_H
#define _hc05_H	

#include "system.h" 


#define HC05_ATSET  	PCout(7) 	//蓝牙控制KEY信号
#define HC05_STATE  	PFin(6)		//蓝牙连接状态信号
  
u8 HC05_Init(void);
u8 HC05_Get_Role(void);
u8 HC05_Set_Cmd(u8* atstr);	

#endif  
















