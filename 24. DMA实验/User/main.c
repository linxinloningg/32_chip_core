

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "key.h"
#include "dma.h"


#define send_buf_len 5000
u8 send_buf[send_buf_len];
/*******************************************************************************
* 函 数 名         : Send_Data
* 函数功能		   : 要发送的数据
* 输    入         : p：指针变量			 
* 输    出         : 无
*******************************************************************************/
void Send_Data(u8 *p)
{
	u16 i;
	for(i=0;i<send_buf_len;i++)
	{
		*p='5';
		p++;
	}
}


/*******************************************************************************
* 函 数 名         : main
* 函数功能		   : 主函数
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
int main()
{
	u8 i=0;
	u8 key;
	
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	KEY_Init();
	DMAx_Init(DMA1_Channel4,(u32)&USART1->DR,(u32)send_buf,send_buf_len);
	Send_Data(send_buf);
	
	while(1)
	{
		key=KEY_Scan(0);
		if(key==KEY_UP)
		{
			USART_DMACmd(USART1,USART_DMAReq_Tx,ENABLE);  //使能串口1的DMA发送     
			DMAx_Enable(DMA1_Channel4,send_buf_len);     //开始一次DMA传输！
		

			//等待DMA传输完成，此时我们来做另外一些事
			//实际应用中，传输数据期间，可以执行另外的任务
			while(1)
			{
				if(DMA_GetFlagStatus(DMA1_FLAG_TC4)!=0)//判断通道4传输完成
				{
					DMA_ClearFlag(DMA1_FLAG_TC4);
					break;
				}
				led2=!led2;
				delay_ms(300);	
			}
		}
		
		i++;
		if(i%20==0)
		{
			led1=!led1;
		}
		
		delay_ms(10);	
	}
}
