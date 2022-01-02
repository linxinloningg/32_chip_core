#include "vs10xx.h"
#include "SysTick.h"
#include "spi.h"

/****************************************************************************
* Function Name  : VS10XX_DelayMs
* Description    : 延时毫秒级函数.
* Input          : ms：延时毫秒数。
* Output         : None
* Return         : None
****************************************************************************/

#define VS10XX_DelayMs(x) {delay_ms(x);}

/****************************************************************************
* Function Name  : VS_Config
* Description    : 对VS10XX芯片使用的IO口和SPI进行初始化.
* Input          : None
* Output         : None
* Return         : None
****************************************************************************/

void VS10XX_Config(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE | RCC_APB2Periph_GPIOG | RCC_APB2Periph_GPIOD, ENABLE);
	
	/* VS10XX_CS 是 PE6 */
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;

	GPIO_Init(GPIOE, &GPIO_InitStructure);
	GPIO_Init(GPIOD, &GPIO_InitStructure); //这个是触摸的片选，为了防止影响选择关闭
    GPIO_SetBits(GPIOD, GPIO_Pin_6);

	/* VS10XX_RESET 是 PG8,S10xx_XDCS 是 PG6*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_8;
	
	GPIO_Init(GPIOG, &GPIO_InitStructure);
	
	/* VS10XX_DREQ 是PG7 */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	
	GPIO_Init(GPIOG, &GPIO_InitStructure);
	
	/* 初始化SPI1 */
	SPI1_Init();
}

/****************************************************************************
* Function Name  : VS10XX_WriteCmd
* Description    : VS10XX写入一个命令.
* Input          : addr：写入地址
*                * cmd：写入命令
* Output         : None
* Return         : 0：写入成功；0xFF：写入失败。
****************************************************************************/

int8_t VS10XX_WriteCmd(uint8_t addr, uint16_t cmd)
{
	uint16_t i = 0;

	while(VS_DREQ == 0)	//等待空闲
	{
		i++;
		if(i > 0xAFFF)
		{
			return 0xFF;
		}
	}

	SPI1_SetSpeed(SPI_BaudRatePrescaler_256);  //设置SPI1低速
	VS_XDCS_SET; 	 
	VS_XCS_CLR;

	SPI1_ReadWriteByte(VS_WRITE_COMMAND);      //发送写命令
	SPI1_ReadWriteByte(addr);				   //发送地址
	SPI1_ReadWriteByte(cmd >> 8);	           //先发高8位
	SPI1_ReadWriteByte(cmd & 0x00FF);
	VS_XCS_SET;

	SPI1_SetSpeed(SPI_BaudRatePrescaler_2);
	return 0;
}

/****************************************************************************
* Function Name  : VS10XX_ReadData
* Description    : VS10XX读取一个数据.
* Input          : addr：要读取的地址
* Output         : None
* Return         : 读取到的16位数据
****************************************************************************/

uint16_t VS10XX_ReadData(uint8_t addr)
{
	uint16_t readValue, i;

	while(VS_DREQ == 0)	//等待空闲
	{
		i++;
		if(i > 0xAFFF)
		{
			return 0xFFFF;
		}
	}

	SPI1_SetSpeed(SPI_BaudRatePrescaler_256);  //设置SPI1低速
	VS_XDCS_SET; 	 
	VS_XCS_CLR;

	SPI1_ReadWriteByte(VS_READ_COMMAND);	   //发送读命令
	SPI1_ReadWriteByte(addr);				   //发送地址
	readValue = SPI1_ReadWriteByte(0xFF);	   //先读高8位
	readValue <<= 8;
	readValue |= SPI1_ReadWriteByte(0xFF);

	VS_XCS_SET;
	SPI1_SetSpeed(SPI_BaudRatePrescaler_2);    //设置SPI1速度

	return readValue;
}

/****************************************************************************
* Function Name  : VS_HardwareReset
* Description    : 硬件复位VS10XX.
* Input          : None
* Output         : None
* Return         : 0：成功；0xFF：失败。
****************************************************************************/
 
int8_t VS10XX_HardwareReset(void)
{
	uint16_t i = 0;

	VS_RST_CLR;
	VS10XX_DelayMs(20);
	VS_XDCS_SET;  //取消数据传输
	VS_XCS_SET;   //取消数据传输
	VS_RST_SET;

	while(VS_DREQ == 0)
	{
		i++;
		if(i > 0x0FFF)
		{
			return 0xFF;
		}	
	}
	VS10XX_DelayMs(20);
	
	return 0;
}

/****************************************************************************
* Function Name  : VS10XX_SoftReset
* Description    : 软件复位VS10xx，同时设置时钟。
* Input          : None
* Output         : None
* Return         : 0：成功；0xFF：失败。
****************************************************************************/

int8_t VS10XX_SoftReset(void)
{
	uint16_t i = 0;
	
	while(VS_DREQ == 0)
	{
		i++;
		if(i > 0x5FFF)
		{
			return 0xFF;
		}	
	}	
	SPI1_ReadWriteByte(0xFF);
	
	i = 0;
	while(VS10XX_ReadData(SCI_MODE) != 0x0800)	// 软件复位,新模式
	{
		VS10XX_WriteCmd(SCI_MODE, 0x0804);      // 软件复位,新模式	    
		VS10XX_DelayMs(2);                      //等待至少1.35ms
		i++;
		if(i > 0x5FFF)
		{
			return 0xFF;
		}	
	}
	
	i = 0;
	while(VS_DREQ == 0)
	{
		i++;
		if(i > 0x5FFF)
		{
			return 0xFF;
		}	
	}

	i = 0;
	while(VS10XX_ReadData(SCI_CLOCKF) != 0X9800) //设置VS10XX的时钟,3倍频 ,1.5xADD 
	{
		VS10XX_WriteCmd(SCI_CLOCKF, 0X9800);     //设置VS10XX的时钟,3倍频 ,1.5xADD
		VS10XX_DelayMs(20);
		i++;
		if(i > 0x5FFF)
		{
			return 0xFF;
		} 	    
	}		    										    
	
	return 0;
}

/****************************************************************************
* Function Name  : VS10XX_RAM_Test
* Description    : 对芯片进行内存测试，读取到0x8000表示测试完成，读取到0x807F
*                * 是VS1003完好，读取到0X83FF是表示VS1053完好。
*                * 测试初始化序列为：0x4D,0xEA,0x6D,0x54,0,0,0,0。
* Input          : None
* Output         : None
* Return         : 0xFFFF：测试失败。或返回测试状态
****************************************************************************/

uint16_t VS10XX_RAM_Test(void)
{
	uint16_t i = 0;

	VS10XX_HardwareReset();
	VS10XX_WriteCmd(SCI_MODE, 0x0820);	// Allow SCI tests 
	
	while(VS_DREQ == 0)
	{
		i++;
		if(i > 0x0FFF)
		{
			return 0xFFFF;
		}	
	}
	SPI1_SetSpeed(SPI_BaudRatePrescaler_256);  //设置SPI1低速
	VS_XDCS_CLR;	       		    // xDCS = 1，选择VS10XX的数据接口

	SPI1_ReadWriteByte(0x4D);
	SPI1_ReadWriteByte(0xEA);
	SPI1_ReadWriteByte(0x6D);
	SPI1_ReadWriteByte(0x54);
	SPI1_ReadWriteByte(0x00);
	SPI1_ReadWriteByte(0x00);
	SPI1_ReadWriteByte(0x00);
	SPI1_ReadWriteByte(0x00);
	
	VS10XX_DelayMs(150);
	VS_XDCS_SET;
	
	return VS10XX_ReadData(SCI_HDAT0); // VS1003得到的值为0x807F;VS1053为0X83FF;			
}

/****************************************************************************
* Function Name  : VS10XX_SineTest
* Description    : 对芯片进行正弦测试.
*                * 进入命令序列为：0x53,0xEF,0x6E,n,0,0,0,0。
*                * 退出命令序列为：0x45,0x78,0x69,0x74,0,0,0,0
* Input          : None
* Output         : None
* Return         : None
****************************************************************************/

void VS10XX_SineTest(void)
{
	uint16_t i;

	VS10XX_HardwareReset();
	VS10XX_WriteCmd(SCI_MODE,0x0820);    // Allow SCI tests
	
	while(VS_DREQ == 0)
	{
		i++;
		if(i > 0x5FFF)
		{
			return ;
		}	
	}
	SPI1_SetSpeed(SPI_BaudRatePrescaler_256);  //设置SPI1低速

	VS_XDCS_CLR;	       		    // xDCS = 1，选择VS10XX的数据接口
	SPI1_ReadWriteByte(0x53);	    //进入正弦测试n = 0x24
	SPI1_ReadWriteByte(0xEF);
	SPI1_ReadWriteByte(0x6E);
	SPI1_ReadWriteByte(0x24);
	SPI1_ReadWriteByte(0x00);
	SPI1_ReadWriteByte(0x00);
	SPI1_ReadWriteByte(0x00);
	SPI1_ReadWriteByte(0x00);
	VS_XDCS_SET;

	VS10XX_DelayMs(100);

	VS_XDCS_CLR;			  // 退出正弦测试
	SPI1_ReadWriteByte(0x45);
	SPI1_ReadWriteByte(0x78);
	SPI1_ReadWriteByte(0x69);
	SPI1_ReadWriteByte(0x74);
	SPI1_ReadWriteByte(0x00);
	SPI1_ReadWriteByte(0x00);
	SPI1_ReadWriteByte(0x00);
	SPI1_ReadWriteByte(0x00);
	VS_XDCS_SET;

	VS10XX_DelayMs(100);
	SPI1_SetSpeed(SPI_BaudRatePrescaler_2);
}

/****************************************************************************
* Function Name  : VS10XX_SendMusicData
* Description    : 发送音乐数据
* Input          : dat：数据缓冲器
* Output         : None
* Return         : 0：发送成功；0xFF：器件忙，未发送成功
****************************************************************************/

uint8_t VS10XX_SendMusicData(uint8_t *dat)
{
    uint8_t i;
    if(VS_DREQ != 0)
    {
        SPI1_SetSpeed(SPI_BaudRatePrescaler_8);
        VS_XDCS_CLR;
        for(i=0; i<32; i++)
        {
            SPI1_ReadWriteByte(dat[i]);
        }
        VS_XDCS_SET;
        return 0;
    }
    else
    {
        return 0xFF;
    }
}


