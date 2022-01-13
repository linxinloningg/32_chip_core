#ifndef _rng_H
#define _rng_H

#include "system.h"

u8 RNG_Init(void);
u32 RNG_Get_RandomNum(void);
int RNG_Get_RandomRange(int min,int max);


#endif
