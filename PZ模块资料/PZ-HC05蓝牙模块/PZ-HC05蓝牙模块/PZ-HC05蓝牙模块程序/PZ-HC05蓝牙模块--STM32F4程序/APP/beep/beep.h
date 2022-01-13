#ifndef _beep_H
#define _beep_H

#include "system.h"

#define BEEP_Pin (GPIO_Pin_8)  //定义BEEP管脚
#define BEEP_Port (GPIOF) //定义BEEP端口

#define beep PFout(8)  //BEEP PF8
void BEEP_Init(void);


#endif
