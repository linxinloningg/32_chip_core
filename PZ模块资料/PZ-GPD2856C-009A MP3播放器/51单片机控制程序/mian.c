/********************************************
*					普中科技
*		GPD2856C-009A MP3播放器模块单片机测试程序
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
		KEY = 0xff;
		if(K1 == 0)
		{
			Delay(100);
			if(K1 == 0)
				Prev = 0;
			while(K1 == 0);
			Delay(100);
		}
		if(K2 == 0)
		{
			Delay(100);
			if(K2 == 0)
				Next = 0;
			while(K2 == 0);	
			Delay(100);
		}
		if(K3 == 0)
		{
			Delay(100);
			if(K3 == 0)
				Mode = 0;
			while(K3 == 0);
			Delay(100);
		}
		if(K4 == 0)
		{
			Delay(100);
			if(K4 == 0)
				RePeat = 0;
			while(K4 == 0);
			Delay(100);
		}
	}
}
