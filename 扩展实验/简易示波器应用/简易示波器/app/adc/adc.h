#ifndef _adc_H
#define _adc_H

#include "system.h"
#include "SysTick.h"

extern u16 a[640];
extern u16 index;
extern u16 index1;

void ADC1_Init(void);
void ADC_Get_Value(void);
u16 ADC_Get_Vpp(void);

#endif
