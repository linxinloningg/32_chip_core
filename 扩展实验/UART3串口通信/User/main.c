#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"




int main()
{
	u16 i=0;
	u8 t;
	u8 len;	
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART3_Init(115200);
	
	while(1)
	{
		if(USART_RX_STA&0x8000)
		{					   
			len=USART_RX_STA&0x3fff;//得到此次接收到的数据长度
			//printf("您发送的消息为: ");
			for(t=0;t<len;t++)
			{
				USART_SendData(USART3, USART_RX_BUF[t]);         //向串口1发送数据
				while(USART_GetFlagStatus(USART3,USART_FLAG_TC)!=SET);//等待发送结束
			}
			USART_RX_STA=0;
			//printf("\r\n");//插入换行
		}
//		else
//		{
//			i++;
//			if(i%5000==0)
//			{
//				printf("普中PZ6808L-F1开发板 USART3通信实验\r\n");
//				printf("www.prechin.cn\r\n");
//			}
//			if(i%200==0)
//				printf("\r\n请输入数据,以回车键结束\r\n");  
//			if(i%20==0)
//				led1=!led1;//闪烁LED,提示系统正在运行.
//			delay_ms(10);   
//		}
	}
}
