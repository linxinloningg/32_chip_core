#include "rng.h"
#include "SysTick.h"

//初始化RNG
//返回值：1：错误，0：成功
u8 RNG_Init(void)
{
	u16 i;
	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG,ENABLE);//使能RNG时钟 AHB2
	RNG_Cmd(ENABLE);//使能RNG
	while(RNG_GetFlagStatus(RNG_FLAG_DRDY)==0)  //等待DRDY稳定
	{
		i++;
		delay_us(100);
		if(i>=10000)
		{
			return 1; //随机数发生器不正常
		}
	}
	return 0;
}

//返回32位随机数
u32 RNG_Get_RandomNum(void)
{
	while(RNG_GetFlagStatus(RNG_FLAG_DRDY)==0);
	return RNG_GetRandomNumber();
}

//生成[min,max]范围的随机数
int RNG_Get_RandomRange(int min,int max)
{
	return min+RNG_Get_RandomNum()%(max-min+1);
}
