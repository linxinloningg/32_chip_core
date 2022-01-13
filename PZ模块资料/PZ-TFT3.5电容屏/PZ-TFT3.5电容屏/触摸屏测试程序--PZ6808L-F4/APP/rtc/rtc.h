#ifndef _rtc_H
#define _rtc_H

#include "system.h"

u8 RTC_Config(void);						//RTC初始化
ErrorStatus RTC_Set_Time(u8 hour,u8 min,u8 sec,u8 ampm);			//RTC时间设置
ErrorStatus RTC_Set_Date(u8 year,u8 month,u8 date,u8 week); 		//RTC日期设置
void RTC_Set_AlarmA(u8 week,u8 hour,u8 min,u8 sec);		//设置闹钟时间(按星期闹铃,24小时制)
void RTC_Set_WakeUp(u32 wksel,u16 cnt);	


#endif
