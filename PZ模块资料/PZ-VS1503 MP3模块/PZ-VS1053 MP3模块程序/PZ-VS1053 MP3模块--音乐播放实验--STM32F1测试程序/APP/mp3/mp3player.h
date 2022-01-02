#ifndef _mp3player_H
#define _mp3player_H


#include "system.h"



						  
void mp3_play(void);
u16 mp3_get_tnum(u8 *path);
u8 mp3_play_song(u8 *pname);
void mp3_index_show(u16 index,u16 total);
void mp3_msg_show(u32 lenth);


#endif












