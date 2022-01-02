#include "mp3player.h"
#include "ff.h"
#include "vs10xx.h"
#include "spi.h"
#include "key.h"
#include "led.h"
#include "tftlcd.h" 

uint8_t MP3_Volume;
/****************************************************************************
* Function Name  : MP3_Init
* Description    : 初始化MP3
* Input          : None
* Output         : None
* Return         : None
****************************************************************************/

int8_t MP3_Init(void)
{
    uint16_t id;
	VS10XX_Config();        //GPIO和SPI初始化
    id = VS10XX_RAM_Test();	
	if(id != 0x83FF)        //检测存储器测试的结果
	{	
		return 0xFF;
	}
	VS10XX_SineTest();     //正弦测试
	MP3_BaseSetting(0,0,0,0);               //基本设置
	MP3_EffectSetting(0);                   //音效设置

	return 0;
}                                 

/****************************************************************************
* Function Name  : MP3_AudioSetting
* Description    : 设置声音的大小，设置SCI_VOL寄存器
* Input          : vol：声音的大小（0~0xFF）
* Output         : None
* Return         : None
****************************************************************************/

void MP3_AudioSetting(uint8_t vol)
{
	uint16_t volValue = 0;
	
	/* 0是最大音量，0xFE是无声，低8字节控制右通道，高8字节控制左通道 */
	vol = 254 - vol;
	volValue = vol | (vol << 8);

	VS10XX_WriteCmd(SCI_VOL, volValue);
}

/****************************************************************************
* Function Name  : MP3_BaseSetting
* Description    : 基本设置：设置SCI_BASS寄存器
* Input          : amplitudeH：高频增益 0~15(单位:1.5dB,小于9的时候为负数)
*                * freqLimitH：高频上限 2~15(单位:10Hz)
*                * amplitudeL：低频增益 0~15(单位:1dB)
*                * freqLimitL：低频下限 1~15(单位:1Khz)
* Output         : None
* Return         : None
****************************************************************************/

void MP3_BaseSetting(
	uint8_t amplitudeH, uint8_t freqLimitH,
	uint8_t amplitudeL, uint8_t freqLimitL
)
{
	uint16_t bassValue = 0;
	
	/* 高频增益是12 ：15位 */
	bassValue = amplitudeH & 0x0F;
	bassValue <<= 4;
	
	/* 频率下限是 11 ：8位 */
	bassValue |= freqLimitL & 0x0F;
	bassValue <<= 4;
	
	/* 低频增益是 7 ：4 位 */
	bassValue |= amplitudeL & 0x0F;
	bassValue <<= 4;
	
	/* 频率上限是 3 ： 0位 */
	bassValue |= freqLimitH & 0x0F;
	
	VS10XX_WriteCmd(SCI_BASS, bassValue); 
		
}

/****************************************************************************
* Function Name  : MP3_EffectSetting
* Description    : 音效设置。设置SCI_MODE寄存器
* Input          : effect：0,关闭;1,最小;2,中等;3,最大。
* Output         : None
* Return         : None
****************************************************************************/

void MP3_EffectSetting(uint8_t effect)
{
	uint16_t effectValue;

	effectValue = VS10XX_ReadData(SCI_MODE);
	if(effect & 0x01)
	{
		effectValue |= 1 << 4;	
	}
	else
	{
		effectValue &= ~(1 << 5);
	}
	if(effect & 0x02)
	{
		effectValue |= 1 << 7;
	}
	else
	{
		effectValue &= ~(1 << 7);
	}

	VS10XX_WriteCmd(SCI_MODE, effectValue);
}

/****************************************************************************
* Function Name  : MP3_ShowVolume
* Description    : 显示音量
* Input          : value：显示的音量
* Output         : None
* Return         : None
****************************************************************************/

void MP3_ShowVolume(uint8_t value)
{
    uint8_t showData[4] = {0, 0, 0, 0};

    showData[0] = value % 1000 / 100 + '0';
    showData[1] = value % 100 / 10 + '0';
    showData[2] = value % 10 + '0';

    FRONT_COLOR=RED;
	LCD_ShowString(96, 84,tftlcd_data.width,tftlcd_data.height,16,showData);
}
/****************************************************************************
* Function Name  : MP3_PlaySong
* Description    : 播放一首歌曲
* Input          : addr：播放地址和歌名（歌曲名记得加.mp3后缀）
* Output         : None
* Return         : None
****************************************************************************/

void MP3_PlaySong(uint8_t *addr)
{
	FIL file;
	UINT br;
	FRESULT res;
	uint8_t musicBuff[512];
	uint16_t k;

    /*open file*/
	res = f_open(&file, (const TCHAR*)addr, FA_READ);
 	led1=0;
	VS10XX_SoftReset();
	led2=0;
	if(res == FR_OK)
	{
		
		while(1)
		{
			res = f_read(&file, musicBuff, sizeof(musicBuff), &br);
			k = 0;

			do
			{	
                /* 发送歌曲信息 */
                if(VS10XX_SendMusicData(musicBuff+k) == 0)
                {
                    k += 32;
                }
                else
                {
                    /* 按键扫描 */
                    switch(KEY_Scan(0))
                    {
                        case(KEY_UP):
                            MP3_Volume++;
                            MP3_AudioSetting(MP3_Volume);
                            MP3_ShowVolume(MP3_Volume);
                            break;
                        case(KEY_DOWN):
                            MP3_Volume--;
                            MP3_AudioSetting(MP3_Volume);
                            MP3_ShowVolume(MP3_Volume);
                            break;
                        case(KEY_RIGHT):
                            return;
                        default:
                            break;
                    }
                    
                }

			}
			while(k < br);
			
			if (res || (br == 0))
			{
				break;    // error or eof 
			} 
		}
		f_close(&file);  //不论是打开，还是新建文件，一定记得关闭
	}		
}

