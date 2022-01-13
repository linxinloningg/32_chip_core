#include "iic.h"

extern uchar BUF[8]; 	//数据缓存

/*******************************************************************************
* 函数名         : Delay_5us()
* 函数功能		 : 延时5us
* 输入           : 无
* 输出         	 : 无
*******************************************************************************/

void Delay_5us()
{
	unsigned char a,b;
	for(b=1;b>0;b--)
		for(a=1;a>0;a--);

}

/**************************************
起始信号
**************************************/
void QMC5883_Start()
{
    SDA = 1;  
    SCL = 1; 
    Delay_5us();     
    SDA = 0;    
    Delay_5us();       
    SCL = 0;      
}

/**************************************
停止信号
**************************************/
void QMC5883_Stop()
{
    SDA = 0;                  
    SCL = 1;                  
    Delay_5us();                
    SDA = 1;                  
    Delay_5us();              
}

/**************************************
发送应答信号
入口参数:ack (0:ACK 1:NAK)
**************************************/
void QMC5883_SendACK(bit ack)
{
    SDA = ack;                
    SCL = 1;                  
    Delay_5us();              
    SCL = 0;                   
    Delay_5us();              
}

/**************************************
接收应答信号
**************************************/
bit QMC5883_RecvACK()
{
    SCL = 1;                  
    Delay_5us();            
    CY = SDA;                 
    SCL = 0;                  
    Delay_5us();               
    return CY;
}

/**************************************
向IIC总线发送一个字节数据
**************************************/
void QMC5883_SendByte(uchar dat)
{
    uchar i;

    for (i=0; i<8; i++)         
    {
        dat <<= 1;              //移出数据的最高位
        SDA = CY;               //送数据口
        SCL = 1;                //拉高时钟线
        Delay_5us();            //延时
        SCL = 0;                //拉低时钟线
        Delay_5us();            //延时
    }
    QMC5883_RecvACK();
}

/**************************************
从IIC总线接收一个字节数据
**************************************/
uchar QMC5883_RecvByte()
{
    uchar i;
    uchar dat = 0;

    SDA = 1;                    //使能内部上拉,准备读取数据,
    for (i=0; i<8; i++)         //8位计数器
    {
        dat <<= 1;
        SCL = 1;                //拉高时钟线
        Delay_5us();            //延时
        dat |= SDA;             //读数据               
        SCL = 0;                //拉低时钟线
        Delay_5us();            //延时
    }
    return dat;
}

//************************写入单字节数据***************************

void Single_Write_QMC5883(uchar REG_Address,uchar REG_data)
{
    QMC5883_Start();                  //起始信号
    QMC5883_SendByte(Slave_Address);  //发送设备地址+写信号
    QMC5883_SendByte(REG_Address);    //内部寄存器地址，请参考中文pdf 
    QMC5883_SendByte(REG_data);       //内部寄存器数据，请参考中文pdf
    QMC5883_Stop();                   //发送停止信号
}

//************************读取单字节数据*************************
//uchar Single_Read_QMC5883(uchar REG_Address)
//{   
//	uchar REG_data;
//	QMC5883_Start();                          //起始信号
//	QMC5883_SendByte(Slave_Address);           //发送设备地址+写信号
//	QMC5883_SendByte(REG_Address);            //发送存储单元地址，从0开始	
//	QMC5883_Start();                          //起始信号
//	QMC5883_SendByte(Slave_Address+1);         //发送设备地址+读信号
//	REG_data=QMC5883_RecvByte();              //读出寄存器数据
//	QMC5883_SendACK(1);   
//	QMC5883_Stop();                           //停止信号
//  return REG_data; 
//}

//******************************************************
//连续读出QMC5883内部角度数据，地址范围0x00~0x05
//******************************************************
void Multiple_Read_QMC5883(void)
{   
    uchar i;
    QMC5883_Start();                          //起始信号
    QMC5883_SendByte(Slave_Address);          //发送设备地址+写信号
    QMC5883_SendByte(0x00);                   //发送存储单元地址，从0x00开始	
    QMC5883_Start();                          //起始信号
    QMC5883_SendByte(Slave_Address+1);        //发送设备地址+读信号
	 for (i=0; i<6; i++)                      //连续读取6个地址数据，存储中BUF
    {
        BUF[i] = QMC5883_RecvByte();          //BUF[0]存储数据
        if (i == 5)
        {
           QMC5883_SendACK(1);                //最后一个数据需要回非应答NOACK
        }
        else
        {
          QMC5883_SendACK(0);                 //应答ACK
       }
   }
    QMC5883_Stop();                           //停止信号
    Delay_ms(5);
}

//初始化QMC5883，根据需要请参考pdf进行修改****
void Init_QMC5883()
{

	Single_Write_QMC5883(0x09,0x0d);  //控制寄存器配置
	Single_Write_QMC5883(0x0b,0x01);  //设置清除时间寄存器
	Single_Write_QMC5883(0x20,0x40);  //
	Single_Write_QMC5883(0x21,0x01);  //	
}





























