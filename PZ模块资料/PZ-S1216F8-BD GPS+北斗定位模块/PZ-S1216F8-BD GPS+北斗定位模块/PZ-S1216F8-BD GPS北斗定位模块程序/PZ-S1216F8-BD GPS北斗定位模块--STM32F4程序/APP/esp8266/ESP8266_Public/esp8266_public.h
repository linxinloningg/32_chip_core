#ifndef _esp8266_public_H
#define _esp8266_public_H


#include "system.h"



//static char *itoa( int value, char *string, int radix );
void USART_printf( USART_TypeDef * USARTx, char * Data, ... );



#endif
