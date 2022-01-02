#include "input.h"

u8 TIM5_CH1_CAPTURE_STA=0; //输入捕获的状态
u32 TIM5_CH1_CAPTURE_VAL=0;//输入捕获值(TIM2/TIM5是32位)

void TIM5_CH1_Input_Init(u32 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_ICInitTypeDef TIM_ICInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5,ENABLE);//使能TIM5时钟
	
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource0,GPIO_AF_TIM5);//管脚复用
	
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_0;//管脚设置
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF; //复用输出模式
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_100MHz;//速度为100M
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_DOWN;//下拉
	GPIO_Init(GPIOA,&GPIO_InitStructure); //初始化结构体
	
	
	TIM_TimeBaseInitStructure.TIM_Period=arr;   //自动装载值
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc; //分频系数
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //设置向上计数模式
	TIM_TimeBaseInit(TIM5,&TIM_TimeBaseInitStructure);	
	
	TIM_ICInitStructure.TIM_Channel=TIM_Channel_1; //通道1
	TIM_ICInitStructure.TIM_ICFilter=0x00;  //滤波
	TIM_ICInitStructure.TIM_ICPolarity=TIM_ICPolarity_Rising;//捕获极性
	TIM_ICInitStructure.TIM_ICPrescaler=TIM_ICPSC_DIV1; //分频系数
	TIM_ICInitStructure.TIM_ICSelection=TIM_ICSelection_DirectTI;//直接映射到TI1
	TIM_ICInit(TIM5,&TIM_ICInitStructure);
	TIM_ITConfig(TIM5,TIM_IT_Update|TIM_IT_CC1,ENABLE);
	
	NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;//中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2;//抢占优先级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;		//子优先级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);
		
	TIM_Cmd(TIM5,ENABLE); //使能定时器
}

void TIM5_IRQHandler(void)
{
	if((TIM5_CH1_CAPTURE_STA&0x80)==0) //还未成功捕获
	{
		if(TIM_GetITStatus(TIM5,TIM_IT_Update)) //发生更新中断
		{
			if(TIM5_CH1_CAPTURE_STA&0X40)//捕获到了高电平
			{
				if((TIM5_CH1_CAPTURE_STA&0x3f)==0x3f) //高电平时间太长
				{
					TIM5_CH1_CAPTURE_STA|=0x80; //标志一次捕获成功
					TIM5_CH1_CAPTURE_VAL=0xffffffff;
				}
				else
				{
					TIM5_CH1_CAPTURE_STA++;
				}
			}
		}
		if(TIM_GetITStatus(TIM5,TIM_IT_CC1)) //发生捕获中断
		{
			if(TIM5_CH1_CAPTURE_STA&0X40)//捕获到了低电平
			{
				TIM5_CH1_CAPTURE_STA|=0x80; //成功捕获一次高电平
				TIM5_CH1_CAPTURE_VAL=TIM_GetCapture1(TIM5);
				TIM_OC1PolarityConfig(TIM5,TIM_ICPolarity_Rising); //设置上升沿捕获
			}
			else
			{
				TIM5_CH1_CAPTURE_STA=0;
				TIM5_CH1_CAPTURE_VAL=0;
				TIM5_CH1_CAPTURE_STA|=0x40; //捕获到高电平 标志
				TIM_Cmd(TIM5,DISABLE);
				TIM_SetCounter(TIM5,0); //定时器初值为0
				TIM_OC1PolarityConfig(TIM5,TIM_ICPolarity_Falling); //设置下降沿捕获
				TIM_Cmd(TIM5,ENABLE);
			}
		}
	}
	TIM_ClearITPendingBit(TIM5,TIM_IT_CC1|TIM_IT_Update);
}


