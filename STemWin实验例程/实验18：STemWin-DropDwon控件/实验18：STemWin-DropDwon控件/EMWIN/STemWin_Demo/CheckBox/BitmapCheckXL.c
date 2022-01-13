#include "GUI.h"
#include "checkboxdemo.h"

/* Colors */
static const GUI_COLOR _aColorDisabled[] = {0x808080, 0x101010};
static const GUI_COLOR _aColorEnabled[]  = {GUI_WHITE, GUI_BLACK};

/* Palettes */
static const GUI_LOGPALETTE _PalCheckDisabled = {
  2,	/* number of entries */
  1, 	/* transparency flag */
  _aColorDisabled
};

static const GUI_LOGPALETTE _PalCheckEnabled = {
  2,	/* number of entries */
  1, 	/* transparency flag */
  _aColorEnabled
};

/* Pixel data */
static const unsigned char _acXLChecked[] = {
  ________, ________,
  ________, ________,
  ________, ________,
  ________, ____XX__,
  ________, ___XXX__,
  ________, __XXXX__,
  ________, _XXXX___,
  ________, XXXX____,
  __XX___X, XXX_____,
  __XXX_XX, XX______,
  __XXXXXX, X_______,
  ___XXXXX, ________,
  ____XXX_, ________,
  ________, ________,
  ________, ________,
};

/* Bitmaps */
const GUI_BITMAP _abmXL[2] = {
  { 16, 16, 2, 1, _acXLChecked, &_PalCheckDisabled },
  { 16, 16, 2, 1, _acXLChecked, &_PalCheckEnabled },
};
