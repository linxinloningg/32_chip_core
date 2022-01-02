/**************************************************
*				   普中科技
*				PZ-QMC5883 数字罗盘测试程序
*	接线方式：	SCL---P10
				SDA---P11
				VCC---VCC
				GND---GND
***************************************************/
#include "reg52.h"
#include "intrins.h"
#include "math.h"
#include "iic.h"
#include "lcd.h"
#include "stdio.h"

uchar ge=0,shi=0,bai=0,qian=0,wan=0;          //显示变量             
uchar BUF[8]=0; 						 //数据缓冲区

//******************************显示在LCD的数据取位********************************//
void Conversion(uint temp_data)  
{  
    wan=temp_data/10000+0x30 ;
    temp_data=temp_data%10000;   //取余运算
	qian=temp_data/1000+0x30 ;
    temp_data=temp_data%1000;    //取余运算
    bai=temp_data/100+0x30   ;
    temp_data=temp_data%100;     //取余运算
    shi=temp_data/10+0x30    ;
    temp_data=temp_data%10;      //取余运算
    ge=temp_data+0x30; 	
}


void main()
{
//	uint i;
	int X=0,Y=0,Z=0;
	double Angle_XY=0,Angle_XZ=0,Angle_YZ=0;	
    Delay_ms(200);
	LcdInit();
	Init_QMC5883();
	Delay_ms(300);
	while(1)            
	{ 
		Multiple_Read_QMC5883();      				//连续读取三轴角度数据，存储在BUF中 
		//---------显示XY轴
		X=BUF[1] << 8 | BUF[0]; //Combine MSB and LSB of X Data output register  最高有效位
		Y=BUF[3] << 8 | BUF[2]; //Combine MSB and LSB of Y Data output register
		Z=BUF[5] << 8 | BUF[4]; //Combine MSB and LSB of Z Data output register
		
		if(X>0x7fff)X-=0xffff;	  
		if(Y>0x7fff)Y-=0xffff;
		if(Z>0x7fff)Z-=0xffff;	 
		
		Display_One_Char(0,0,'Q');
		Display_One_Char(1,0,'M');
		Display_One_Char(2,0,'C');
		Display_One_Char(3,0,'5');
		Display_One_Char(4,0,'8');
		Display_One_Char(5,0,'8');
		Display_One_Char(6,0,'3');

		Angle_XY= atan2((double)Y,(double)X) * (180 / 3.14159265) + 180; //计算XY平面角度
		Angle_XY*=10;
		Conversion(Angle_XY);       //计算XY平面角度数据和显示
		Display_One_Char(8,0,'X');
		Display_One_Char(9,0,'Y');
		Display_One_Char(10,0,':'); 
		Display_One_Char(11,0,qian); 
		Display_One_Char(12,0,bai); 
		Display_One_Char(13,0,shi); 
		Display_One_Char(14,0,'.'); 
		Display_One_Char(15,0,ge); 
		
		Angle_XZ= atan2((double)Z,(double)X) * (180 / 3.14159265) + 180; //计算XZ平面角度
		Angle_XZ*=10;
		Conversion(Angle_XZ);       //计算XZ平面角度数据和显示
		Display_One_Char(0,1,'X');
		Display_One_Char(1,1,'Z');
		Display_One_Char(2,1,':'); 
		Display_One_Char(3,1,qian); 
		Display_One_Char(4,1,bai); 
		Display_One_Char(5,1,shi); 
		Display_One_Char(6,1,'.'); 
		Display_One_Char(7,1,ge); 
		
		Angle_YZ= atan2((double)Z,(double)Y) * (180 / 3.14159265) + 180; //计算YZ平面角度
		Angle_YZ*=10;
		Conversion(Angle_YZ);       //计算YZ平面角度数据和显示
		Display_One_Char(8,1,'Y');
		Display_One_Char(9,1,'Z');
		Display_One_Char(10,1,':'); 
		Display_One_Char(11,1,qian); 
		Display_One_Char(12,1,bai); 
		Display_One_Char(13,1,shi); 
		Display_One_Char(14,1,'.'); 
		Display_One_Char(15,1,ge); 
	
		Delay_ms(1);   //延时                     
  }
}
