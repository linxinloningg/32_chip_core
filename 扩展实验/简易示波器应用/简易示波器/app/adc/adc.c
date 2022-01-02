#include "adc.h"
#include "math.h"
#include "tim.h"
#include "stm32f10x_it.h"

#define ADC1_DR_Address    ((u32)0x4001244C)

u16 a[640];
u16 index = 0;
u16 index1 = 0;


void DMA1_Init(void)
{
	DMA_InitTypeDef  DMA_InitTypeStruct;

	DMA_DeInit(DMA1_Channel1);	//ADC1的DMA通道

	DMA_InitTypeStruct.DMA_PeripheralBaseAddr = ADC1_DR_Address;
	DMA_InitTypeStruct.DMA_MemoryBaseAddr = (u32)a;
	DMA_InitTypeStruct.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitTypeStruct.DMA_BufferSize = 640;
	DMA_InitTypeStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitTypeStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitTypeStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitTypeStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitTypeStruct.DMA_Mode = DMA_Mode_Normal;
	DMA_InitTypeStruct.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitTypeStruct.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel1,&DMA_InitTypeStruct); //DMA初始化，ADC结果寄存器存入SRAM中

	DMA_Cmd(DMA1_Channel1,ENABLE);	//DMA使能
}

void AD_Init(void)
{
	ADC_InitTypeDef  ADC_InitTypeStruct;

	ADC_DeInit(ADC1);  //复位ADC1

	ADC_InitTypeStruct.ADC_Mode = ADC_Mode_Independent;//ADC工作在独立模式
	ADC_InitTypeStruct.ADC_ScanConvMode = DISABLE;	   //单通道模式
	ADC_InitTypeStruct.ADC_ContinuousConvMode = DISABLE; //单次模式
	ADC_InitTypeStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;//选择定时器1来触发
	ADC_InitTypeStruct.ADC_DataAlign = ADC_DataAlign_Right; //数据右对齐
	ADC_InitTypeStruct.ADC_NbrOfChannel = 1;   //通道数目为1
	ADC_Init(ADC1, &ADC_InitTypeStruct);	  //ADC初始化

	ADC_ExternalTrigConvCmd(ADC1,ENABLE);	  //ADC外部触发使能
	ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 1, ADC_SampleTime_1Cycles5);//配置ADC1，通道1等
}

void ADC1_Init(void)									 //adc初始化
{
	DMA1_Init();
	AD_Init();
	ADC_DMACmd(ADC1, ENABLE);    //ADC DMA使能

	ADC_Cmd(ADC1, ENABLE);		 //ADC使能
	ADC_ResetCalibration(ADC1);	 //ADC校准复位
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);	 //ADC开始校准
	while(ADC_GetCalibrationStatus(ADC1));
}

void ADC_Get_Value(void)								 //得到数据，
{
	
	float gao_pin_period = 0;
	DMA1_Init();	                  
	TIM_SetCounter(TIM1,0);	   //清空寄存器
	if(num_shao_miao>7)
	{
		TIM_PrescalerConfig(TIM1,71,TIM_PSCReloadMode_Immediate);
		TIM_SetCompare1(TIM1, (shao_miao_shu_du/25)-1);
		TIM_SetAutoreload(TIM1, (shao_miao_shu_du/25)-1); //设定扫描速度
	}
	else
	{
		TIM_PrescalerConfig(TIM1,0,TIM_PSCReloadMode_Immediate);

		gao_pin_period = 288000000.0/frequency +gao_pin_palus-1;

		TIM_SetCompare1(TIM1, gao_pin_period);
		TIM_SetAutoreload(TIM1,gao_pin_period);	
	}
	
	TIM_Cmd(TIM1,ENABLE);
	while(!DMA_GetFlagStatus(DMA1_FLAG_TC1));
	DMA_ClearFlag(DMA1_FLAG_TC1);
	if(DMA_GetFlagStatus(DMA1_FLAG_TE1))
	{
		DMA_ClearFlag(DMA1_FLAG_TE1);
		ADC_Get_Value();
	}
	TIM_Cmd(TIM1,DISABLE);
}

u16 ADC_Get_Vpp(void)	   
{
	u32 max_data=a[0];
	u32 min_data=a[0];
	u32 n=0;
	float pp=0;
	for(n = 1;n<320;n++)
	{
		if(a[n]>max_data)
		{
			max_data = a[n];
		}
		if(a[n]<min_data)
		{
			min_data = a[n];
			index = n;
		}			
	} 	
	pp = (float)(max_data-min_data);
	pp = pp*(3300.0/4095);
	return pp;	
}
