#include "Scrollbarwin_demo.h"
#include "GUI.h"
#include "SCROLLBAR.h"
#include "EDIT.h"


#define EDIT_MAX_X 10
#define EDIT_MAX_Y 3

static EDIT_Handle _aahEdit[EDIT_MAX_Y][EDIT_MAX_X];
static int         _x;

/*********************************************************************
*
*       _cbWindow
*
* Function description
*   The callback moves the edit-fiels when a notification message was send.
*/
static void _cbWindow(WM_MESSAGE * pMsg) {
  WM_SCROLL_STATE ScrollState;
  int             x;
  int             y;

  switch (pMsg->MsgId) {
  case WM_NOTIFY_PARENT:
    if (pMsg->Data.v == WM_NOTIFICATION_VALUE_CHANGED) {
      if (WM_GetId(pMsg->hWinSrc) == GUI_ID_HSCROLL) {
        WM_GetScrollState(pMsg->hWinSrc, &ScrollState);
        if (_x != ScrollState.v) {
          for (y = 0; y < EDIT_MAX_Y; y++) {
            for (x = 0; x < EDIT_MAX_X; x++) {
              WM_MoveWindow(_aahEdit[y][x], _x - ScrollState.v, 0);
            }
          }
          _x = ScrollState.v;
        }
      }
    }
    break;
  case WM_PAINT:
    GUI_SetBkColor(GUI_BLACK);
    GUI_Clear();
    break;
  default:
    WM_DefaultProc(pMsg);
  }
}

/*********************************************************************
*
*       _DemoScrollbarMove
*
* Function description
*   This function creates the window and his child objects. Then it
*   runs into a idle-loop, so that the window manager can handle the
*   objects.
*/
static void _DemoScrollbarMove(void) {
  SCROLLBAR_Handle hScroll;
  WM_HWIN          hWindow;
  int              x;
  int              y;

  //
  // Clear display and display headline
  //
  GUI_SetBkColor(GUI_BLUE);
  GUI_Clear();
  GUI_SetColor(GUI_WHITE);
  GUI_SetFont(&GUI_Font24_ASCII);
  GUI_SetTextAlign(GUI_TA_HCENTER);
  GUI_DispStringAt("SCROLLBAR_Move - Sample", 160, 5);
  GUI_SetTextAlign(GUI_TA_LEFT);
  GUI_SetColor(0xFFFFFF);
  GUI_SetFont(&GUI_Font8x16);
  //
  // Create the window
  //
  hWindow = WM_CreateWindow(50, 90, 220, 79, WM_CF_SHOW, _cbWindow, 0);
  //
  // Create the scrollbar
  //
  hScroll = SCROLLBAR_CreateAttached(hWindow, 0);
  SCROLLBAR_SetNumItems(hScroll, 48 * EDIT_MAX_X);
  SCROLLBAR_SetPageSize(hScroll, 220);
  //
  // Create the edit-fields
  //
  for (y = 0; y < EDIT_MAX_Y; y++) {
    for (x = 0; x < EDIT_MAX_X; x++) {
      _aahEdit[y][x] = EDIT_CreateAsChild(x * 48, y * 22, 48, 22, hWindow, 13, WM_CF_SHOW, 5);
      EDIT_SetTextAlign(_aahEdit[y][x], GUI_TA_RIGHT | GUI_TA_VCENTER);
      EDIT_SetFont(_aahEdit[y][x], &GUI_Font8x16);
      EDIT_SetDecMode(_aahEdit[y][x], ((y * EDIT_MAX_X) + x) * 100, -99999, +99999, 2, 0);
	  }
  }
}

void STemWin_ScrollbarWin_Test(void)
{
	_DemoScrollbarMove();
	while(1) 
	{
		GUI_Delay(20);
	}
}	

