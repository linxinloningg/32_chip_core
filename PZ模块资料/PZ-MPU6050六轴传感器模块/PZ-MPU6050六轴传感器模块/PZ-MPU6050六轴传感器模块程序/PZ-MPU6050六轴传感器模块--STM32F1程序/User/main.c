

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "tftlcd.h"
#include "key.h"
#include "mpu6050.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h" 



//串口1发送1个字符 
//c:要发送的字符
void usart1_send_char(u8 c)
{
	while(USART_GetFlagStatus(USART1,USART_FLAG_TC)==RESET); 
    USART_SendData(USART1,c);   

} 

//传送数据给匿名四轴上位机软件(V2.6版本)
//fun:功能字. 0XA0~0XAF
//data:数据缓存区,最多28字节!!
//len:data区有效数据个数
void usart1_niming_report(u8 fun,u8*data,u8 len)
{
	u8 send_buf[32];
	u8 i;
	if(len>28)return;	//最多28字节数据 
	send_buf[len+3]=0;	//校验数置零
	send_buf[0]=0X88;	//帧头
	send_buf[1]=fun;	//功能字
	send_buf[2]=len;	//数据长度
	for(i=0;i<len;i++)send_buf[3+i]=data[i];			//复制数据
	for(i=0;i<len+3;i++)send_buf[len+3]+=send_buf[i];	//计算校验和	
	for(i=0;i<len+4;i++)usart1_send_char(send_buf[i]);	//发送数据到串口1 
}

//发送加速度传感器数据和陀螺仪数据
//aacx,aacy,aacz:x,y,z三个方向上面的加速度值
//gyrox,gyroy,gyroz:x,y,z三个方向上面的陀螺仪值
void mpu6050_send_data(short aacx,short aacy,short aacz,short gyrox,short gyroy,short gyroz)
{
	u8 tbuf[12]; 
	tbuf[0]=(aacx>>8)&0XFF;
	tbuf[1]=aacx&0XFF;
	tbuf[2]=(aacy>>8)&0XFF;
	tbuf[3]=aacy&0XFF;
	tbuf[4]=(aacz>>8)&0XFF;
	tbuf[5]=aacz&0XFF; 
	tbuf[6]=(gyrox>>8)&0XFF;
	tbuf[7]=gyrox&0XFF;
	tbuf[8]=(gyroy>>8)&0XFF;
	tbuf[9]=gyroy&0XFF;
	tbuf[10]=(gyroz>>8)&0XFF;
	tbuf[11]=gyroz&0XFF;
	usart1_niming_report(0XA1,tbuf,12);//自定义帧,0XA1
}	

//通过串口1上报结算后的姿态数据给电脑
//aacx,aacy,aacz:x,y,z三个方向上面的加速度值
//gyrox,gyroy,gyroz:x,y,z三个方向上面的陀螺仪值
//roll:横滚角.单位0.01度。 -18000 -> 18000 对应 -180.00  ->  180.00度
//pitch:俯仰角.单位 0.01度。-9000 - 9000 对应 -90.00 -> 90.00 度
//yaw:航向角.单位为0.1度 0 -> 3600  对应 0 -> 360.0度
void usart1_report_imu(short aacx,short aacy,short aacz,short gyrox,short gyroy,short gyroz,short roll,short pitch,short yaw)
{
	u8 tbuf[28]; 
	u8 i;
	for(i=0;i<28;i++)tbuf[i]=0;//清0
	tbuf[0]=(aacx>>8)&0XFF;
	tbuf[1]=aacx&0XFF;
	tbuf[2]=(aacy>>8)&0XFF;
	tbuf[3]=aacy&0XFF;
	tbuf[4]=(aacz>>8)&0XFF;
	tbuf[5]=aacz&0XFF; 
	tbuf[6]=(gyrox>>8)&0XFF;
	tbuf[7]=gyrox&0XFF;
	tbuf[8]=(gyroy>>8)&0XFF;
	tbuf[9]=gyroy&0XFF;
	tbuf[10]=(gyroz>>8)&0XFF;
	tbuf[11]=gyroz&0XFF;	
	tbuf[18]=(roll>>8)&0XFF;
	tbuf[19]=roll&0XFF;
	tbuf[20]=(pitch>>8)&0XFF;
	tbuf[21]=pitch&0XFF;
	tbuf[22]=(yaw>>8)&0XFF;
	tbuf[23]=yaw&0XFF;
	usart1_niming_report(0XAF,tbuf,28);//飞控显示帧,0XAF
} 




int main()
{	
	u8 i=0;
	u8 key;
	u8 report=1;
	float pitch,roll,yaw; 		//欧拉角
	short aacx,aacy,aacz;		//加速度传感器原始数据
	short gyrox,gyroy,gyroz;	//陀螺仪原始数据
	short temp;					//温度
	u8 res;
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(256000);
	TFTLCD_Init();			//LCD初始化
	KEY_Init();
	MPU6050_Init();					//初始化MPU6050
	
	FRONT_COLOR=BLACK;
	LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"PRECHIN STM32F1");
	LCD_ShowString(10,30,tftlcd_data.width,tftlcd_data.height,16,"www.prechin.net");
	LCD_ShowString(10,50,tftlcd_data.width,tftlcd_data.height,16,"MPU6050 Test");
	
	FRONT_COLOR=RED;
	res=mpu_dmp_init();
	printf("res=%d\r\n",res);
	while(mpu_dmp_init())
	{
		printf("MPU6050 Error!\r\n");
		LCD_ShowString(10,130,tftlcd_data.width,tftlcd_data.height,16,"MPU6050 Error!");
		delay_ms(200);
	}
	printf("MPU6050 OK!\r\n");
	LCD_ShowString(10,130,tftlcd_data.width,tftlcd_data.height,16,"MPU6050 OK!  ");
	LCD_ShowString(10,150,tftlcd_data.width,tftlcd_data.height,16,"K_UP:UPLOAD ON/OFF");
	FRONT_COLOR=BLUE;//设置字体为蓝色 
 	LCD_ShowString(10,170,tftlcd_data.width,tftlcd_data.height,16,"UPLOAD ON ");	 
 	LCD_ShowString(10,200,tftlcd_data.width,tftlcd_data.height,16," Temp:    . C");	
 	LCD_ShowString(10,220,tftlcd_data.width,tftlcd_data.height,16,"Pitch:    . C");	
 	LCD_ShowString(10,240,tftlcd_data.width,tftlcd_data.height,16," Roll:    . C");	 
 	LCD_ShowString(10,260,tftlcd_data.width,tftlcd_data.height,16," Yaw :    . C");	
	
	while(1)
	{
		key=KEY_Scan(0);
		if(key==KEY_UP)
		{
			report=!report;
			if(report)LCD_ShowString(10,170,tftlcd_data.width,tftlcd_data.height,16,"UPLOAD ON ");
			else LCD_ShowString(10,170,tftlcd_data.width,tftlcd_data.height,16,"UPLOAD OFF");
		}
		
		if(mpu_dmp_get_data(&pitch,&roll,&yaw)==0)
		{ 
			temp=MPU6050_Get_Temperature();	//得到温度值
			MPU6050_Get_Accelerometer(&aacx,&aacy,&aacz);	//得到加速度传感器数据
			MPU6050_Get_Gyroscope(&gyrox,&gyroy,&gyroz);	//得到陀螺仪数据
			if(report)mpu6050_send_data(aacx,aacy,aacz,gyrox,gyroy,gyroz);//用自定义帧发送加速度和陀螺仪原始数据
			if(report)usart1_report_imu(aacx,aacy,aacz,gyrox,gyroy,gyroz,(int)(roll*100),(int)(pitch*100),(int)(yaw*10));
			if((i%10)==0)
			{ 
				if(temp<0)
				{
					LCD_ShowChar(10+48,200,'-',16,0);		//显示负号
					temp=-temp;		//转为正数
				}else LCD_ShowChar(10+48,200,' ',16,0);		//去掉负号 
				printf("检测温度为：%d度\r\n",temp);
				LCD_ShowNum(10+48+8,200,temp/100,3,16);		//显示整数部分	    
				LCD_ShowNum(10+48+40,200,temp%10,1,16);		//显示小数部分 
				temp=pitch*10;
				if(temp<0)
				{
					LCD_ShowChar(10+48,220,'-',16,0);		//显示负号
					temp=-temp;		//转为正数
				}else LCD_ShowChar(10+48,220,' ',16,0);		//去掉负号 
				printf("Pitch：%d\r\n",temp);
				LCD_ShowNum(10+48+8,220,temp/10,3,16);		//显示整数部分	    
				LCD_ShowNum(10+48+40,220,temp%10,1,16);		//显示小数部分 
				temp=roll*10;
				if(temp<0)
				{
					LCD_ShowChar(10+48,240,'-',16,0);		//显示负号
					temp=-temp;		//转为正数
				}else LCD_ShowChar(10+48,240,' ',16,0);		//去掉负号 
				printf("Roll：%d\r\n",temp);
				LCD_ShowNum(10+48+8,240,temp/10,3,16);		//显示整数部分	    
				LCD_ShowNum(10+48+40,240,temp%10,1,16);		//显示小数部分 
				temp=yaw*10;
				if(temp<0)
				{
					LCD_ShowChar(10+48,260,'-',16,0);		//显示负号
					temp=-temp;		//转为正数
				}else LCD_ShowChar(10+48,260,' ',16,0);		//去掉负号 
				printf("Yaw：%d\r\n",temp);
				LCD_ShowNum(10+48+8,260,temp/10,3,16);		//显示整数部分	    
				LCD_ShowNum(10+48+40,260,temp%10,1,16);		//显示小数部分  
				led1=!led1;
			}
		}		
		i++;			
	}
}

