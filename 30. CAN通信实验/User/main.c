

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "key.h"
#include "can.h"


/*******************************************************************************
* 函 数 名         : main
* 函数功能		   : 主函数
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
int main()
{
	u8 i=0,j=0;
	u8 key;
	u8 mode=0;
	u8 res;
	u8 tbuf[8],char_buf[8];
	u8 rbuf[8];

	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	KEY_Init();
	CAN_Mode_Init(CAN_SJW_1tq,CAN_BS2_8tq,CAN_BS1_9tq,4,CAN_Mode_Normal);//500Kbps波特率
	
	while(1)
	{
		key=KEY_Scan(0);
		if(key==KEY_UP)  //模式切换
		{
			mode=!mode;
			CAN_Mode_Init(CAN_SJW_1tq,CAN_BS2_8tq,CAN_BS1_9tq,4,mode);
			if(mode==0)
			{
				printf("Normal Mode\r\n");
			}
			else
			{
				printf("LoopBack Mode\r\n");
			}
	
		}
		if(key==KEY_DOWN)  //发送数据
		{
			for(j=0;j<8;j++)
			{
				tbuf[j]=j;
				char_buf[j]=tbuf[j]+0x30;
			}
			res=CAN_Send_Msg(tbuf,8);
			if(res)
			{
				printf("Send Failed!\r\n");
			}
			else
			{
				printf("发送数据：%s\r\n",char_buf);
			}
			
		}
		res=CAN_Receive_Msg(rbuf);
		if(res)
		{
			for(j=0;j<res;j++)
			{
				char_buf[j]=rbuf[j]+0x30;
			}
			printf("接收数据：%s\r\n",char_buf);
		}
		
		i++;
		if(i%20==0)
		{
			led1=!led1;
		}
		
		delay_ms(10);
	}
}
