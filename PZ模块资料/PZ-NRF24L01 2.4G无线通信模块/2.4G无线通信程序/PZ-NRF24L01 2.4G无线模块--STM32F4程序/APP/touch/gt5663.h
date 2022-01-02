#ifndef _gt5663_H
#define _gt5663_H


#include "system.h"


//与电容触摸屏连接的芯片引脚(未包含IIC引脚) 
//IO操作函数	 
#define GT_RST    				PCout(13)	//复位引脚
#define GT_INT    				PBin(1)		//中断引脚	



//I2C读写命令	
#define GT_CMD_WR 		0X28     	//写命令0X28
#define GT_CMD_RD 		0X29		//读命令0X29
  
//GT5663 部分寄存器定义 
#define GT_CTRL_REG 	0X8040   	//GT5663控制寄存器
#define GT_CFGS_REG 	0X8050   	//GT5663配置起始地址寄存器0X8050
#define GT_CHECK_REG 	0X8042   	//GT5663校验和寄存器0X8042
#define GT_PID_REG 		0X8140   	//GT5663产品ID寄存器

#define GT_GSTID_REG 	0X814E   	//GT5663当前检测到的触摸情况
#define GT_TP1_REG 		0X8150  	//第一个触摸点数据地址
#define GT_TP2_REG 		0X8158		//第二个触摸点数据地址
#define GT_TP3_REG 		0X8160		//第三个触摸点数据地址
#define GT_TP4_REG 		0X8168		//第四个触摸点数据地址
#define GT_TP5_REG 		0X8170		//第五个触摸点数据地址  
 
 
u8 GT5663_Send_Cfg(u8 mode);
u8 GT5663_WR_Reg(u16 reg,u8 *buf,u8 len);
void GT5663_RD_Reg(u16 reg,u8 *buf,u8 len); 
u8 GT5663_Init(void);
u8 GT5663_Scan(u8 mode);

#endif
