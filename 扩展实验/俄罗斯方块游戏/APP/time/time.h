#ifndef _time_H
#define _time_H

#include "system.h"

extern u8 key;
void TIM4_Init(u16 per,u16 psc);
void TIM3_Init(u16 per,u16 psc);
#endif
