#include "graph2d_demo.h"
#include "GUI.h"
#include "math.h"
#include "stdlib.h"
#include "stdio.h"

void STemWin_2DDraw_Test(void)
{
	GUI_SetBkColor(GUI_BLUE);
	GUI_Clear();
	GUI_SetFont(&GUI_Font16_ASCII);
	GUI_DispStringHCenterAt("PRECHIN 2D_Draw Test",200,0);
	GUI_SetColor(GUI_RED);
	GUI_SetFont(&GUI_Font13_ASCII);	
	
	GUI_SetBkColor(GUI_YELLOW);
	GUI_ClearRect(10,20,100,80);
	
	GUI_SetBkColor(GUI_BLUE);

	//绘制水平颜色梯度填充矩形
	GUI_DrawGradientH(120,20,210,80,0x4117BB,0xc6b6f5);
	
	//绘制垂直颜色梯度填充圆角矩形
	GUI_DrawGradientRoundedV(230,20,320,80,20,0x4117BB,0xc6b6f5);
	
	GUI_DrawRect(10,100,100,160);  //指定位置画矩形
	
	GUI_FillRect(120,100,210,160);  //指定位置填充矩形

	GUI_FillRoundedRect(230,100,320,160,20);  //指定填充圆角矩形
}

void STemWin_Alpha_Test(void)  //Alpha测试
{
	GUI_SetBkColor(GUI_WHITE);
	GUI_Clear();
	GUI_SetFont(&GUI_Font16_ASCII);
	GUI_SetColor(GUI_BLACK);	
	GUI_DispStringHCenterAt("PRECHIN Alpha Test",200,0);

	GUI_EnableAlpha(1);	 //开启Alpha
	GUI_SetColor(GUI_BLACK);
	GUI_DispStringHCenterAt("Alpha Test",45,61);

	GUI_SetAlpha(40);
	GUI_SetColor(GUI_RED);
	GUI_FillRect(0,20,49,69);

	GUI_SetAlpha(80);
	GUI_SetColor(GUI_GREEN);
	GUI_FillRect(20,40,69,89);

	GUI_SetAlpha(120);
	GUI_SetColor(GUI_BLUE);
	GUI_FillRect(40,60,89,109);

	GUI_EnableAlpha(0);  //关闭Alpha

}

const GUI_POINT point[]=
{
	{10,220},
	{56,100},
	{102,220},
	{148,100},
	{192,220}
};
void STemWin_Line_Test(void)   //线条绘制显示测试
{
	GUI_SetBkColor(GUI_BLUE);
	GUI_Clear();
	GUI_SetFont(&GUI_Font16_ASCII);
	GUI_SetColor(GUI_RED);	
	GUI_DispStringHCenterAt("PRECHIN DrawLine Test",200,0);

	GUI_DrawHLine(30,10,250);  //绘制水平线
	
	GUI_SetPenSize(3);
	GUI_DrawLine(10,50,250,50);  //绘制直线

	GUI_SetPenSize(1);  //画虚线需要将画笔的大小调节到1
	GUI_SetLineStyle(GUI_LS_DASH);  //设置虚线模式
	GUI_DrawLine(10,70,250,70); 

	GUI_SetLineStyle(GUI_LS_SOLID);  //设置实线模式
	GUI_DrawVLine(300,30,200);   //绘制垂直线
	
	GUI_SetPenSize(2);
	GUI_DrawPolyLine(point,GUI_COUNTOF(point),0,0);
}

const GUI_POINT point2[]=
{
	{40,20},
	{0,20},
	{20,0}
};
const GUI_POINT point3[]=
{
	{0,-5},
	{-40,-35},
	{-10,-25},
	{-10,-85},
	{10,-85},
	{10,-25},
	{40,-35}
};

GUI_POINT enlargepoint[GUI_COUNTOF(point2)];
void STemWin_2DGraph_Test(void)   //图形绘制显示测试
{
	int i;
	int j;
	int x=0,y=20;
	int x0=160,y0=160;
	char ac[4];
	I16 ay[100];
	int a0,a1;
	const unsigned avalue[]={100,135,190,240,340,360};
	const GUI_COLOR acolor[]={GUI_BLACK,GUI_GREEN,GUI_RED,GUI_CYAN,GUI_MAGENTA,GUI_YELLOW};

	GUI_SetBkColor(GUI_BLUE);
	GUI_Clear();
	GUI_SetFont(&GUI_Font16_ASCII);
	GUI_SetColor(GUI_WHITE);	
	GUI_DispStringHCenterAt("PRECHIN 2DGraph Test",200,0);
	
	GUI_SetColor(GUI_RED);	
	GUI_SetDrawMode(GUI_DM_XOR);
	GUI_FillPolygon(point2,GUI_COUNTOF(point2),140,110);  //绘制填充多边形轮廓
	for(i=1;i<5;i++)
	{
		GUI_EnlargePolygon(enlargepoint,point2,GUI_COUNTOF(point2),i*5);  //扩展多边形
		GUI_FillPolygon(enlargepoint,GUI_COUNTOF(point2),140,110);
	}
	GUI_Delay(1000);

	GUI_Clear();
	GUI_FillPolygon(point2,GUI_COUNTOF(point2),x,y);
	for(i=1;i<5;i++)
	{	
		GUI_MagnifyPolygon(enlargepoint,point2,GUI_COUNTOF(point2),i); //放大多边形
		GUI_FillPolygon(enlargepoint,GUI_COUNTOF(point2),x,y);
		x+=40*i;
	}
	GUI_Delay(1000);

	//箭头的绘制
	GUI_Clear();
	GUI_SetColor(GUI_RED);   
	GUI_SetFont(&GUI_Font8x16);
	GUI_DispStringAt("Polygons of arbitrary shap ",0,0);
	GUI_DispStringAt("in any color",120,20);
	GUI_FillPolygon(point3,GUI_COUNTOF(point3),100,100);
	GUI_Delay(1000);

	//绘制圆
	GUI_Clear();
	GUI_SetColor(GUI_YELLOW);
	GUI_DispStringHCenterAt("PRECHIN CircleTest",200,0);
	for(i=0;i<100;i+=10)
	{
		GUI_DrawCircle(100,100,i);  //画圆
	}
	GUI_FillCircle(300,100,50);  //填充圆
	GUI_Delay(1000);

   //绘制椭圆
	GUI_SetColor(GUI_YELLOW);
	GUI_DispStringHCenterAt("PRECHIN Ellipse Test",200,0);
	for(i=0;i<100;i+=10)
	{
		for(j=0;j<100;j+=10)
		{
			GUI_DrawEllipse(100,100,i,j);
		}
	}
	for(i=0,j=0;i<100,j<50;i+=20,j+=10)
	{
		GUI_DrawEllipse(300,100,i,j);	
	}
	GUI_Delay(1000);

	//绘制弧线  简单的仪表盘
	GUI_SetBkColor(GUI_BLUE);
	GUI_Clear();
	GUI_SetPenSize(5);
	GUI_SetTextMode(GUI_TM_TRANS);
	GUI_SetFont(&GUI_FontComic18B_ASCII);
	GUI_SetColor(GUI_YELLOW);
	GUI_DrawArc(x0,y0,150,150,-30,210);
	GUI_DispStringHCenterAt("PRECHIN Arc Test",300,0);
	GUI_Delay(1000);
	GUI_DrawPoint(x0,y0);
	for(i=0;i<23;i++)
	{
		float a=(-30+i*10)*3.1415926/180;
		int x1=-141*cos(a)+x0;
		int y1=-141*sin(a)+y0;
		if(i%2==0)
		{
			GUI_SetPenSize(5);
		}
		else
		{
			GUI_SetPenSize(4);
		}
		GUI_DrawPoint(x1,y1);
		if(i%2==0)
		{
			x1=-123*cos(a)+x0;
			y1=-130*sin(a)+y0;
			sprintf(ac,"%d",10*i);
			GUI_SetTextAlign(GUI_TA_VCENTER);
			GUI_DispStringHCenterAt(ac,x1,y1);
		}
		
	}
	GUI_Delay(1000);


	//绘制饼图
	GUI_SetBkColor(GUI_BLUE);
	GUI_Clear();
	GUI_SetFont(&GUI_Font16_ASCII);
	GUI_SetColor(GUI_YELLOW);
	GUI_DispStringHCenterAt("PRECHIN Pie Test",200,20);
	for(i=0;i<GUI_COUNTOF(avalue);i++)
	{
		a0=(i==0)?0:avalue[i-1];
		a1=avalue[i];
		GUI_SetColor(acolor[i]);
		GUI_DrawPie(100,100,50,a0,a1,0);
	}
	GUI_Delay(1000);


	//绘制线图
	GUI_SetBkColor(GUI_BLUE);
	GUI_Clear();
	GUI_SetFont(&GUI_Font16_ASCII);
	GUI_SetColor(GUI_YELLOW);
	GUI_DispStringHCenterAt("PRECHIN Graph Test",200,20);
	
	while(1)
	{
		GUI_Clear();
		GUI_DispStringHCenterAt("PRECHIN Graph Test",200,20);
		for(i=0;i<GUI_COUNTOF(ay);i++)
		{
			ay[i]=rand()%100;
		}
		GUI_DrawGraph(ay,GUI_COUNTOF(ay),0,30);	
		GUI_Delay(1000);
	}			
}

void STemWIN_Graph2D_Test(void)   
{
	//可以选择释放注释代码，一个个测试具体的2D图形显示函数功能
//	STemWin_2DDraw_Test();
//	STemWin_Alpha_Test();
//	STemWin_Line_Test();
	STemWin_2DGraph_Test();
	while(1)
	{
		GUI_Delay(10);
	}

}

