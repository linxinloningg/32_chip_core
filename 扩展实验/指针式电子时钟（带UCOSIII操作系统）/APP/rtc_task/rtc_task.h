#ifndef _rtc_task_H
#define _rtc_task_H

#include "system.h"

void draw_circle(void);	 //ª≠‘≤
void draw_dotline(void);  //ª≠∏Òµ„
void draw_hand(int hhour,int mmin,int ssec);  //ª≠÷∏’Î
void draw_hand_clear(int hhour,int mmin,int ssec);  //≤¡÷∏’Î
void RTC_task(void *p_arg);


#endif
