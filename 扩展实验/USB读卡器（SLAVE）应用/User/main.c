/* 下载程序后，先要插上SD卡，将USB模块短接片短接到USB端，用USB线连接PC的USB口和STM32 USB口，
	即可自动安装USB驱动识别SD卡及Flash内存。在PC机我的电脑内即可看到有2个硬盘。D1指示灯闪烁
	表示程序正常运行，当对SD卡或者Flash读写的时候D2亮，LCD显示状态*/

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "tftlcd.h"
#include "key.h"
#include "malloc.h" 
#include "sd.h"
#include "flash.h"
#include "ff.h" 
#include "fatfs_app.h"
#include "font_show.h"


//移植USB时候的头文件
#include "mass_mal.h"
#include "usb_lib.h"
#include "hw_config.h"
#include "usb_pwr.h"
#include "memory.h"	    
#include "usb_bot.h"


//USB使能连接/断线
//enable:0,断开
//       1,允许连接	   
void USB_Port_Set(u8 enable)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);    //使能PORTA时钟		 
	if(enable)_SetCNTR(_GetCNTR()&(~(1<<1)));//退出断电模式
	else
	{	  
		_SetCNTR(_GetCNTR()|(1<<1));  // 断电模式
		GPIOA->CRH&=0XFFF00FFF;
		GPIOA->CRH|=0X00033000;
		PAout(12)=0;	    		  
	}
} 

int main()
{
	u8 offline_cnt=0;
	u8 tct=0;
	u8 USB_STA;
	u8 Divece_STA;
	u32 sd_size;
	u8 sd_buf[6];
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	TFTLCD_Init();			//LCD初始化
	KEY_Init();
	EN25QXX_Init();				//初始化EN25Qxx  
	
	my_mem_init(SRAMIN);		//初始化内部内存池
	
	FRONT_COLOR=RED;//设置字体为红色 	
	LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,16,"USB Card Reader TEST");	
	LCD_ShowString(10,30,tftlcd_data.width,tftlcd_data.height,16,"www.prechin.net");
	
	if(SD_Init()!=0)
	{	
		LCD_ShowString(10,50,tftlcd_data.width,tftlcd_data.height,16,"SD Card Error!");	//检测SD卡错误
	}
	else //SD 卡正常
	{   															  
		LCD_ShowString(10,50,tftlcd_data.width,tftlcd_data.height,16,"SD Card Size:     MB"); 
 		Mass_Memory_Size[0]=(long long)SD_GetSectorCount()*512;//得到SD卡容量（字节），当SD卡容量超过4G的时候,需要用到两个u32来表示
	    Mass_Block_Size[0] =512;//因为我们在Init里面设置了SD卡的操作字节为512个,所以这里一定是512个字节.
	    Mass_Block_Count[0]=Mass_Memory_Size[0]/Mass_Block_Size[0];
		sd_size=Mass_Block_Count[0]>>11;  //显示SD卡总容量   MB
		sd_buf[0]=sd_size/10000+0x30;
		sd_buf[1]=sd_size%10000/1000+0x30;
		sd_buf[2]=sd_size%10000%1000/100+0x30;
		sd_buf[3]=sd_size%10000%1000%100/10+0x30;
		sd_buf[4]=sd_size%10000%1000%100%10+0x30;
		sd_buf[5]='\0';
		LCD_ShowString(10+13*8,50,tftlcd_data.width,tftlcd_data.height,16,sd_buf); 
 	}
	if(EN25QXX_ReadID()!=EN25Q64)
	{
		LCD_ShowString(10,70,tftlcd_data.width,tftlcd_data.height,16,"EN25QXX Error!          ");	//检测EN25QXX错误
	}
	else //SPI FLASH 正常
	{   														 	 
		Mass_Memory_Size[1]=1024*1024*6;//前6M字节
	    Mass_Block_Size[1] =512;//因为我们在Init里面设置了SD卡的操作字节为512个,所以这里一定是512个字节.
	    Mass_Block_Count[1]=Mass_Memory_Size[1]/Mass_Block_Size[1];
		LCD_ShowString(10,70,tftlcd_data.width,tftlcd_data.height,16,"EN25QXX FLASH Size:6144KB");
	} 
	
 	delay_ms(1800);
 	USB_Port_Set(0); 	//USB先断开
	delay_ms(300);
   	USB_Port_Set(1);	//USB再次连接	   
 	LCD_ShowString(10,90,tftlcd_data.width,tftlcd_data.height,16,"USB Connecting...");//提示正在建立连接 	    
	printf("USB Connecting...\r\n");	 
   	//USB配置
 	USB_Interrupts_Config();    
 	Set_USBClock();   
 	USB_Init();	    
	delay_ms(1800);
	
	while(1)
	{
		delay_ms(1);
		if(USB_STA!=USB_STATUS_REG)//状态改变了 
		{	 						   			  	   
			if(USB_STATUS_REG&0x01)//正在写		  
			{
				led2=0;
				LCD_ShowString(10,120,tftlcd_data.width,tftlcd_data.height,16,"USB Writing...");//提示USB正在写入数据	 
				printf("USB Writing...\r\n"); 
			}
			if(USB_STATUS_REG&0x02)//正在读
			{
				led2=0;
				LCD_ShowString(10,120,tftlcd_data.width,tftlcd_data.height,16,"USB Reading...");//提示USB正在读出数据  		 
				printf("USB Reading...\r\n"); 		 
			}	 										  
			if(USB_STATUS_REG&0x04)
				LCD_ShowString(10,140,tftlcd_data.width,tftlcd_data.height,16,"USB Write Err ");//提示写入错误
			else 
				LCD_Fill(10,140,tftlcd_data.width,140+16,WHITE);//清除显示	  
			if(USB_STATUS_REG&0x08)
				LCD_ShowString(10,140,tftlcd_data.width,tftlcd_data.height,16,"USB Read  Err ");//提示读出错误
			else 
				LCD_Fill(10,140,tftlcd_data.width,140+16,WHITE);//清除显示      
			USB_STA=USB_STATUS_REG;//记录最后的状态
		}
		if(Divece_STA!=bDeviceState) 
		{
			if(bDeviceState==CONFIGURED)
			{
				LCD_ShowString(10,90,tftlcd_data.width,tftlcd_data.height,16,"USB Connected    ");//提示USB连接已经建立	
				printf("USB Connected\r\n");
			}
			else 
			{
				LCD_ShowString(10,90,tftlcd_data.width,tftlcd_data.height,16,"USB DisConnected ");//提示USB被拔出了
				printf("USB DisConnected\r\n");
			}
			Divece_STA=bDeviceState;
		}
		tct++;
		if(tct==200)
		{
			tct=0;
			led2=1;
			led1=!led1;//提示系统在运行
			if(USB_STATUS_REG&0x10)
			{
				offline_cnt=0;//USB连接了,则清除offline计数器
				bDeviceState=CONFIGURED;
			}
			else//没有得到轮询 
			{
				offline_cnt++;  
				if(offline_cnt>10)bDeviceState=UNCONNECTED;//2s内没收到在线标记,代表USB被拔出了
			}
			USB_STATUS_REG=0;
		}
	}	
}

