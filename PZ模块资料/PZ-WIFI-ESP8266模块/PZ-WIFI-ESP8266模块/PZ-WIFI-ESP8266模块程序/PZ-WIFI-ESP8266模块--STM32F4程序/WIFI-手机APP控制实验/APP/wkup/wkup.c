#include "wkup.h"

void Enter_Standby_Mode(void)
{
		
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR,ENABLE);//使能PWR外设时钟
	
	//PWR_BackupAccessCmd(ENABLE);//后备区域访问使能
	//如果后面使用RTC中断功能，这里需要关闭相关RTC中断
	//（等到后面使用RTC功能，如果需要待机唤醒就需要开启下面两条注释的语句）
//	RTC_ITConfig(RTC_IT_TS|RTC_IT_WUT|RTC_IT_ALRB|RTC_IT_ALRA,DISABLE);//关闭RTC相关中断，可能在RTC实验打开了。
//	RTC_ClearITPendingBit(RTC_IT_TS|RTC_IT_WUT|RTC_IT_ALRB|RTC_IT_ALRA);//清除RTC相关中断标志位。
	
	PWR_ClearFlag(PWR_FLAG_WU);//清除Wake-up 标志
	
	PWR_WakeUpPinCmd(ENABLE);//使能唤醒管脚	使能或者失能唤醒管脚功能
	
	
	PWR_EnterSTANDBYMode();//进入待机模式
}



