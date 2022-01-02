

#include "stm32f10x.h"
#include "led.h"


/*******************************************************************************
* 函 数 名         : delay
* 函数功能		   : 延时函数，通过while循环占用CPU，达到延时功能
* 输    入         : i
* 输    出         : 无
*******************************************************************************/
void delay(u32 i)
{
	while(i--);
}

/*******************************************************************************
* 函 数 名         : RCC_HSE_Config
* 函数功能		   : 自定义系统时钟，可以通过修改PLL时钟源和倍频系数实现时钟调整
* 输    入         : div：RCC_PLLSource_HSE_Div1/RCC_PLLSource_HSE_Div2
					 pllm：RCC_PLLMul_2-RCC_PLLMul_16
* 输    出         : 无
*******************************************************************************/
void RCC_HSE_Config(u32 div,u32 pllm) //自定义系统时间（可以修改时钟）
{
	RCC_DeInit(); //将外设RCC寄存器重设为缺省值
	RCC_HSEConfig(RCC_HSE_ON);//设置外部高速晶振（HSE）
	if(RCC_WaitForHSEStartUp()==SUCCESS) //等待HSE起振
	{
		RCC_HCLKConfig(RCC_SYSCLK_Div1);//设置AHB时钟（HCLK）
		RCC_PCLK1Config(RCC_HCLK_Div2);//设置低速AHB时钟（PCLK1）
		RCC_PCLK2Config(RCC_HCLK_Div1);//设置高速AHB时钟（PCLK2）
		RCC_PLLConfig(div,pllm);//设置PLL时钟源及倍频系数
		RCC_PLLCmd(ENABLE); //使能或者失能PLL
		while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY)==RESET);//检查指定的RCC标志位设置与否,PLL就绪
		RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);//设置系统时钟（SYSCLK）
		while(RCC_GetSYSCLKSource()!=0x08);//返回用作系统时钟的时钟源,0x08：PLL作为系统时钟
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
	RCC_HSE_Config(RCC_PLLSource_HSE_Div2,RCC_PLLMul_9);   //36M
	LED_Init();
	while(1)
	{
		GPIO_ResetBits(LED_PORT,GPIO_Pin_0);//点亮D1
		delay(6000000);
		GPIO_SetBits(LED_PORT,GPIO_Pin_0);
		delay(6000000);
	}
}
