#include "beep.h"

//初始化蜂鸣器
void BEEP_Init(void) 
{
	GPIO_InitTypeDef GPIO_InitStructure; //定义结构体变量	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF,ENABLE); //使能端口F时钟
	
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_OUT; //输出模式
	GPIO_InitStructure.GPIO_Pin=BEEP_Pin;//管脚设置F8
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_100MHz;//速度为100M
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_UP;//上拉
	GPIO_Init(BEEP_Port,&GPIO_InitStructure); //初始化结构体
	
	GPIO_SetBits(BEEP_Port,BEEP_Pin); //关闭蜂鸣器
	
}

