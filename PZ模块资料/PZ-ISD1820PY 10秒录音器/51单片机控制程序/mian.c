/********************************************
*					普中科技
*		PZ-ISD1820录音模块模块单片机测试程序
********************************************/

#include "reg51.h"

//定义IO口
#define KEY P1

sbit Prev 	=	P1^0;
sbit Next 	=	P1^1;
sbit Mode	=	P1^2;
sbit RePeat	=	P1^3;

sbit K1		=	P1^4;
sbit K2		=	P1^5;
sbit K3		=	P1^6;
sbit K4		=	P1^7;

//延时函数
void Delay(unsigned int x)
{
	unsigned int i,j;
	for(i=x;i>0;i--)
		for(j=120;j>0;j--);
}

//主函数
void main( void )
{
	while(1)
	{	
		KEY = 0xf0;
		if(K1 == 0)
		{
			Delay(100);
			if(K1 == 0)
				Prev = 1;
			while(K1 == 0);
			Delay(100);
		}
		if(K2 == 0)
		{
			Delay(100);
			if(K2 == 0)
				Next = 1;
			while(K2 == 0);	
			Delay(100);
		}
		if(K3 == 0)
		{
			Delay(100);
			if(K3 == 0)
				Mode = 1;
			while(K3 == 0);
			Delay(100);
		}
		if(K4 == 0)
		{
			Delay(100);
			if(K4 == 0)
				RePeat = 1;
			while(K4 == 0);
			Delay(100);
		}
	}
}
