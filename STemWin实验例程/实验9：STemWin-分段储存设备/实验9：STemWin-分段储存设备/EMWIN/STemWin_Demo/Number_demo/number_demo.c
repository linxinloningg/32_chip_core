#include "number_demo.h"
#include "GUI.h"

void STemWin_NumDec_Test(void)   //十进制数字显示测试
{
	GUI_SetBkColor(GUI_BLUE);
	GUI_Clear();
	GUI_SetFont(&GUI_Font16_ASCII);
	GUI_SetColor(GUI_RED);
	GUI_DispStringHCenterAt("PRECHIN NumDec Test",200,0);
	GUI_SetFont(&GUI_Font13_ASCII);
	GUI_DispStringAt("GUI_DispDec(): ",0,20);
	
	GUI_DispDec(123,4);  //首位可以显示0
	GUI_DispString("   ");
	GUI_DispDec(-123,4);
	
	GUI_DispStringAt("GUI_DispDecAt(): ",0,35);
	GUI_DispDecAt(100,100,35,3);
	GUI_DispDecAt(-100,130,35,4);
	GUI_DispString("    ");
	GUI_DispDecMin(300);  //当前位置显示十进制数，不需要指定长度

	GUI_DispStringAt("GUI_DispDecShift(): ",0,50);
	GUI_DispDecShift(1234,5,2);   //带小数显示数字
	GUI_DispString("    ");
	GUI_DispDecShift(-1234,6,3);

	GUI_DispStringAt("GUI_DispDecSpace(): ",0,65);
	GUI_DispDecSpace(5678,5);   //不支持首位为0的显示 位数多余用空格补齐
	GUI_DispString("    ");
	GUI_DispDecSpace(-5678,5);

	GUI_DispStringAt("GUI_DispSDec(): ",0,80);
	GUI_DispSDec(12,3);  //带符号显示数字
	GUI_DispString("    ");
	GUI_DispSDec(-34,3);

	GUI_DispStringAt("GUI_DispSDecShift(): ",0,95);
	GUI_DispSDecShift(123,5,2);
	GUI_DispString("    ");
	GUI_DispSDecShift(-123,5,2);

}

void STemWin_NumFloat_Test(void)   //浮点数显示测试
{
	float f=123.45;
	GUI_SetBkColor(GUI_BLUE);
	GUI_Clear();
	GUI_SetFont(&GUI_Font16_ASCII);
	GUI_DispStringHCenterAt("PRECHIN NumFloat Test",200,0);
	GUI_SetColor(GUI_RED);
	GUI_SetFont(&GUI_Font13_ASCII);
	
	GUI_DispStringAt("GUI_DispFloat(): ",0,20);
	GUI_DispFloat(f,6);   //显示浮点数  不支持首位为0格式
	GUI_GotoX(140);
	GUI_DispFloat(-f,7);

	GUI_DispStringAt("GUI_DispFloatFix(): ",0,35);
	GUI_DispFloatFix(f,6,2);  //支持首位为0格式
	GUI_GotoX(140);
	GUI_DispFloatFix(f,7,2);
	GUI_GotoX(200);
	GUI_DispFloatFix(-f,8,3);

	GUI_DispStringAt("GUI_DispSFloatFix(): ",0,50);
	GUI_DispSFloatFix(f,7,2);
	GUI_DispString("    ");
	GUI_DispSFloatFix(-f,7,2);

	GUI_DispStringAt("GUI_DispFloatMin(): ",0,65);
	GUI_DispFloatMin(f,3);
	GUI_DispString("    ");
	GUI_DispFloatMin(-f,3);

	GUI_DispStringAt("GUI_DispSFloatMin(): ",0,65);
	GUI_DispSFloatMin(f,3);
	GUI_DispString("    ");
	GUI_DispSFloatMin(-f,3);
	
}

void STemWin_BinHex_Test(void)  //二进制和16进制显示测试
{
	int dec=20;
	char *emwinversion;
	GUI_SetBkColor(GUI_BLUE);
	GUI_Clear();
	GUI_SetFont(&GUI_Font16_ASCII);
	GUI_DispStringHCenterAt("PRECHIN Bin&Hex Test",200,0);
	GUI_SetColor(GUI_RED);
	GUI_SetFont(&GUI_Font13_ASCII);
	
	GUI_DispStringAt("GUI_DispBin(): ",0,20);	
	GUI_DispBin(dec,8);
	
	GUI_DispStringAt("GUI_DispBinAt(): ",0,35);	
	GUI_DispBinAt(dec,100,35,8);

	GUI_DispStringAt("GUI_DispHex(): ",0,50);	
	GUI_DispHex(dec,2);

	GUI_DispStringAt("GUI_DispHexAt(): ",0,65);	
	GUI_DispHexAt(dec,100,65,2);

	GUI_DispStringAt("GUI_GetVersionString(): ",0,100);
	emwinversion=(char*)GUI_GetVersionString();
	GUI_DispString(emwinversion);

}

void STemWin_Num_Test(void)
{	
	while(1)
	{
		STemWin_NumDec_Test();
		GUI_Delay(2000);
		STemWin_NumFloat_Test();
		GUI_Delay(2000);
		STemWin_BinHex_Test();
		GUI_Delay(2000);
	}
}

