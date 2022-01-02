#ifndef _crc_H
#define _crc_H

#include "system.h"




extern u32 CRCValue;

void CRC_Init(void);
u32 CRC_GetValue(void);

#endif
