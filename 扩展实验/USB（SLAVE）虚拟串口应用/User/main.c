/* 
	做USB实验的时候一定要将USB端的短接片短接到USB侧，如果使用CAN则需要短接到CAN侧。
	使用USB线插在开发板USB模块接口上（注意不是USB串口下载口），此时电脑会自动识别安装驱动，
	如果未识别请手动安装在“\5--开发工具\4. 常用辅助开发软件”文件夹内的STM32 USB虚拟串口驱动
	才能使用该例程。
*/

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "tftlcd.h"
#include "key.h"
#include "malloc.h" 



//移植USB时候的头文件
#include "usb_lib.h"
#include "hw_config.h"
#include "usb_pwr.h"



int main()
{
	u16 len=0,times=0,t;
	u8 usbstatus=0;
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	TFTLCD_Init();			//LCD初始化
	
	delay_ms(1800);
	USB_Port_Set(0); 	//USB先断开
	delay_ms(700);
	USB_Port_Set(1);	//USB再次连接
	Set_USBClock();
	USB_Interrupts_Config();
	USB_Init();
	while(1)
	{
		if(usbstatus!=bDeviceState)//USB连接状态发生了改变.
		{
			usbstatus=bDeviceState;//记录新的状态
			if(usbstatus==CONFIGURED)
			{
				LCD_ShowString(10,90,tftlcd_data.width,tftlcd_data.height,16,"USB Connected    ");//提示USB连接已经建立
				usb_printf("USB Connected\r\n");
				led2=0;//D2亮
			}
			else
			{
				LCD_ShowString(10,90,tftlcd_data.width,tftlcd_data.height,16,"USB DisConnected ");//提示USB被拔出了
				usb_printf("USB disConnected\r\n");
				led2=1;//D2灭
			}
		}
		if(USB_USART_RX_STA&0x8000)
		{					   
			len=USB_USART_RX_STA&0x3FFF;//得到此次接收到的数据长度
			usb_printf("\r\n您发送的消息长度为:%d\r\n\r\n",len);
			for(t=0;t<len;t++)
			{
				USB_USART_SendData(USB_USART_RX_BUF[t]);//以字节方式,发送给USB 
			}
			usb_printf("\r\n\r\n");//插入换行
			USB_USART_RX_STA=0;
		}
		else
		{
			times++;
			if(times%5000==0)
			{
				usb_printf("\r\n普中科技STM32开发板USB虚拟串口实验\r\n");
				usb_printf("www.prechin.com\r\n\r\n");
			}
			if(times%200==0)usb_printf("请输入数据,以回车键结束\r\n");  
			if(times%30==0)led1=!led1;//闪烁LED,提示系统正在运行.
			delay_ms(10);   
		}					
	}	
}

