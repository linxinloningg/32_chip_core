#include "time.h"
#include "led.h"
#include "tftlcd.h"
#include "key.h"
#include "ball.h"


/*******************************************************************************
* 函 数 名         : TIM4_Init
* 函数功能		   : TIM4初始化函数
* 输    入         : per:重装载值
					 psc:分频系数
* 输    出         : 无
*******************************************************************************/
void TIM4_Init(u16 per,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4,ENABLE);//使能TIM4时钟
	
	TIM_TimeBaseInitStructure.TIM_Period=per;   //自动装载值
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc; //分频系数
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //设置向上计数模式
	TIM_TimeBaseInit(TIM4,&TIM_TimeBaseInitStructure);
	
	TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE); //开启定时器中断
	TIM_ClearITPendingBit(TIM4,TIM_IT_Update);
	
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;//定时器中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0;//抢占优先级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =2;		//子优先级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	
	
	TIM_Cmd(TIM4,ENABLE); //使能定时器	
}


/*******************************************************************************
* 函 数 名         : TIM4_IRQHandler
* 函数功能		   : TIM4中断函数
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
void TIM4_IRQHandler(void)
{
	static u8 i=0,j=0;
	if(TIM_GetITStatus(TIM4,TIM_IT_Update))
	{
		if(game.life)
		{
			GUI_DotP(pai.x,pai.y,BLACK);   //消除显示
			button=KEY_Scan(1);
			if(button==KEY_LEFT)
			{
				i++;
				if(i>=3)
				{
					i=0;
					pai.x-=5;
					if(pai.x<=1)pai.x=1;
				}	
			}
			if(button==KEY_RIGHT)
			{
				j++;
				if(j>=3)
				{
					j=0;
					pai.x+=5;
					if(pai.x>=209)pai.x=209;
				}	
			}
			GUI_DotP(pai.x,pai.y,WHITE);   //显示新的拍子
		}		
	}
	TIM_ClearITPendingBit(TIM4,TIM_IT_Update);	
}


void TIM3_Init(u16 per,u16 psc)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //时钟使能
	
	//定时器TIM3初始化
	TIM_TimeBaseInitStructure.TIM_Period=per;   //自动装载值
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc; //分频系数
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //设置向上计数模式
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStructure);
	
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE ); //使能指定的TIM3中断,允许更新中断

	//中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  //TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //初始化NVIC寄存器
	TIM_Cmd(TIM3, ENABLE);  //使能TIMx					 
}

void TIM3_IRQHandler(void)
{
	static u8 i=0;
	if(TIM_GetITStatus(TIM3,TIM_IT_Update))
	{
		if(game.life)
		{
			GUI_DotB(ball.x,ball.y,BLACK);	//清除上一个点
				
			switch(ball.dir)		//运行方向判断
			{
				case 1: ball.x+=1,ball.y-=1;break;	  //右上角 
				case 2: ball.x-=1,ball.y-=1;break;	  //左上角 
				case 3: ball.x-=1,ball.y+=1;break;	  //左下角 
				case 4: ball.x+=1,ball.y+=1;break;	  //右下角
			}
			GUI_DotB(ball.x,ball.y,WHITE);	//球新的位子显示
			
			if(ball.y==1)  //当球碰到上边界 碰到边界也有2种方向判断
			{
				if(ball.dir==1)		//右上角方向碰到
				{
					ball.dir=4;  //反弹方向	
				}
				else
				{
					ball.dir=3;
				}
			}
		
			if((ball.y==pai.y-5)&&(ball.x>=pai.x-5&&ball.x<=pai.x+30))  //当球碰到下边界 碰到边界也有2种方向判断
			{
				if(ball.dir==4)		//右下角方向碰到
				{
					ball.dir=1;  //反弹方向	
				}
				else
				{
					ball.dir=2;
				}
				game.sco=1;
				game.score+=10;  //得分	
			}
			if(ball.y>pai.y-5)
			{
				game.life=0; //游戏结束
			}
			if(ball.x==234)  //当球碰到右边界 碰到边界也有2种方向判断
			{
				if(ball.dir==1)		//右上角方向碰到
				{
					ball.dir=2;  //反弹方向	
				}
				else
				{
					ball.dir=3;
				}
			}
		
			if(ball.x==1)  //当球碰到左边界 碰到边界也有2种方向判断
			{
				if(ball.dir==3)		//左下角方向碰到
				{
					ball.dir=4;  //反弹方向	
				}
				else
				{
					ball.dir=1;
				}
			}
			i++;
			if(i==30)
			{
				i=0;
				led2=!led2;
			}		
		}		
	}
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
}


void TIM5_Init(u16 per,u16 psc)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE); //时钟使能
	
	//定时器TIM初始化
	TIM_TimeBaseInitStructure.TIM_Period=per;   //自动装载值
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc; //分频系数
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //设置向上计数模式
	TIM_TimeBaseInit(TIM5,&TIM_TimeBaseInitStructure);
	
	TIM_ITConfig(TIM5,TIM_IT_Update,ENABLE ); //使能指定的TIM中断,允许更新中断

	//中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;  //TIM中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;  //从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //初始化NVIC寄存器
	TIM_Cmd(TIM5, ENABLE);  //使能TIMx					 
}

void TIM5_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM5,TIM_IT_Update)!= RESET)
	{
		button=KEY_Scan(1);
	}
	TIM_ClearITPendingBit(TIM5,TIM_IT_Update);
}


