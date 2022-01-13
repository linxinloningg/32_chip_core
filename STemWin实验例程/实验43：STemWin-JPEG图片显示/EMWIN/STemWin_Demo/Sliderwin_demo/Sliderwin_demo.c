#include "Sliderwin_demo.h"
#include "GUI.h"
#include "DIALOG.h"




/*********************************************************************
*
*       Dialog resource
*
* Function description
*   This table conatins the info required to create the dialog.
*   It has been created manually, but could also be created by a GUI-builder.
*/
static const GUI_WIDGET_CREATE_INFO _aDialogCreate[] = {
  { FRAMEWIN_CreateIndirect, "Adjust color", 0,         10,  40, 300, 160, FRAMEWIN_CF_MOVEABLE},
  { TEXT_CreateIndirect,     "Red:" ,  0,                5,  20,  35,  20, TEXT_CF_LEFT },
  { TEXT_CreateIndirect,     "Green:", 0,                5,  50,  35,  20, TEXT_CF_LEFT },
  { TEXT_CreateIndirect,     "Blue:",  0,                5,  80,  35,  20, TEXT_CF_LEFT },
  { TEXT_CreateIndirect,     "Preview",0,              205,   4,  81,  15, TEXT_CF_HCENTER },
  { SLIDER_CreateIndirect,   NULL,     GUI_ID_SLIDER0,  40,  20, 100,  20 },
  { SLIDER_CreateIndirect,   NULL,     GUI_ID_SLIDER1,  40,  50, 100,  20 },
  { SLIDER_CreateIndirect,   NULL,     GUI_ID_SLIDER2,  40,  80, 100,  20 },
  { EDIT_CreateIndirect,     NULL,     GUI_ID_EDIT0,   145,  20,  30,  20, 0, 3 },
  { EDIT_CreateIndirect,     NULL,     GUI_ID_EDIT1,   145,  50,  30,  20, 0, 3 },
  { EDIT_CreateIndirect,     NULL,     GUI_ID_EDIT2,   145,  80,  30,  20, 0, 3 },
  { BUTTON_CreateIndirect,   "OK",     GUI_ID_OK,       10, 110,  60,  20 },
  { BUTTON_CreateIndirect,   "Cancel", GUI_ID_CANCEL,  230, 110,  60,  20 },
};


static U8 _aColorSep[3] = {0, 127, 255};  // Red, green and blue components

/*********************************************************************
*
*       _OnPaint
*
* Function description
*   This routine draws the color rectangles.
*   The widgets are drawn automatically.
*/
static void _OnPaint(void) {
  //
  // Draw RGB values
  //
  GUI_SetColor(_aColorSep[0]);
  GUI_FillRect(180, 20, 199, 39);
  GUI_SetColor(_aColorSep[1] << 8);
  GUI_FillRect(180, 50, 199, 69);
  GUI_SetColor(((U32)_aColorSep[2])<<16 );
  GUI_FillRect(180, 80, 199, 99);
  //
  // Draw resulting color
  //
  GUI_SetColor(_aColorSep[0] | (((U32)_aColorSep[1]) << 8) | (((U32)_aColorSep[2]) << 16));
  GUI_FillRect(205, 20, 285, 99);
}

/*********************************************************************
*
*       _OnValueChanged
*/
static void _OnValueChanged(WM_HWIN hDlg, int Id) {
  unsigned Index;
  unsigned v;
  WM_HWIN  hSlider;
  WM_HWIN  hEdit;

  Index = 0;
  v     = 0;
  if ((Id >= GUI_ID_SLIDER0) && (Id <= GUI_ID_SLIDER2)) {
    Index = Id - GUI_ID_SLIDER0;
    //
    // SLIDER-widget has changed, update EDIT-widget
    //
    hSlider = WM_GetDialogItem(hDlg, GUI_ID_SLIDER0 + Index);
    hEdit   = WM_GetDialogItem(hDlg, GUI_ID_EDIT0 + Index);
    v = SLIDER_GetValue(hSlider);
    EDIT_SetValue(hEdit, v);
  } else if ((Id >= GUI_ID_EDIT0) && (Id <= GUI_ID_EDIT2)) {
    Index = Id - GUI_ID_EDIT0;
    //
    // If EDIT-widget has changed, update SLIDER-widget
    //
    hSlider = WM_GetDialogItem(hDlg, GUI_ID_SLIDER0 + Index);
    hEdit   = WM_GetDialogItem(hDlg, GUI_ID_EDIT0 + Index);
    v = EDIT_GetValue(hEdit);
    SLIDER_SetValue(hSlider, v);
  }
  _aColorSep[Index] = v;
  //
  // At last invalidate dialog client window
  //
  //WM_InvalidateWindow(WM_GetClientWindow(hDlg));
}

/*********************************************************************
*
*       _cbBkWindow
*/
static void _cbBkWindow(WM_MESSAGE * pMsg) {
  
  switch (pMsg->MsgId) {
  case WM_PAINT:
    GUI_SetBkColor(GUI_WHITE);
    GUI_Clear();
    GUI_SetColor(GUI_RED);
    GUI_SetFont(&GUI_Font24_ASCII);
    GUI_DispStringHCenterAt("DIALOG_SliderColor - Sample", 160, 5);
    
  default:
    WM_DefaultProc(pMsg);
  }
}

/*********************************************************************
*
*       _cbCallback
*/
static void _cbCallback(WM_MESSAGE * pMsg) {
  WM_HWIN hDlg;
  WM_HWIN hItem;
  int     i;
  int     NCode;
  int     Id;

  hDlg = pMsg->hWin;
  switch (pMsg->MsgId) {
    case WM_PAINT:
      _OnPaint();
      return;
    case WM_INIT_DIALOG:
      for (i = 0; i < 3; i++) {
        hItem = WM_GetDialogItem(hDlg, GUI_ID_SLIDER0 + i);
        SLIDER_SetRange(hItem, 0, 255);
        SLIDER_SetValue(hItem, _aColorSep[i]);
        //
        // Init EDIT-widgets
        //
        hItem = WM_GetDialogItem(hDlg, GUI_ID_EDIT0 + i);
        EDIT_SetDecMode(hItem, _aColorSep[i],   0, 255, 0, 0);
      }
      break;
    case WM_KEY:
      switch (((WM_KEY_INFO*)(pMsg->Data.p))->Key) {
        case GUI_KEY_ESCAPE:
          ;
          break;
        case GUI_KEY_ENTER:
          GUI_EndDialog(hDlg, 0);
          break;
      }
      break;
    case WM_NOTIFY_PARENT:
      Id    = WM_GetId(pMsg->hWinSrc);      // Id of widget
      NCode = pMsg->Data.v;                 // Notification code
      switch (NCode) {
        case WM_NOTIFICATION_RELEASED:      // React only if released
          if (Id == GUI_ID_OK) {            // OK Button
            GUI_EndDialog(hDlg, 0);
          }
          if (Id == GUI_ID_CANCEL) {        // Cancel Button
            GUI_EndDialog(hDlg, 1);
          }
          break;
        case WM_NOTIFICATION_VALUE_CHANGED: // Value has changed
          _OnValueChanged(hDlg, Id);
          break;
      }
      break;
    default:
      WM_DefaultProc(pMsg);
  }
}


void STemWin_SliderWin_Test(void)
{
	WM_SetCallback(WM_HBKWIN, _cbBkWindow);  
	WM_SetCreateFlags(WM_CF_MEMDEV);  // Use memory devices on all windows to avoid flicker
	while (1) 
	{
		GUI_ExecDialogBox(_aDialogCreate, GUI_COUNTOF(_aDialogCreate), &_cbCallback, 0, 0, 0);
		GUI_Delay(1000);
	}
}	
