/*
*********************************************************************************************************
*                                     MICRIUM BOARD SUPPORT SUPPORT
*
*                          (c) Copyright 2003-2009; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*               Knowledge of the source code may NOT be used to develop a similar product.
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                     MICIUM BOARD SUPPORT PACKAGE
*                                 STLM75 CMOS TEMPERATURE SENSOR DRIVER
*                                                                         
*
* Filename      : bsp_stlm75.h
* Version       : V1.00
* Programmer(s) : FT
*********************************************************************************************************
* Note(s)       :
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                                 MODULE
*
* Note(s) : (1) This header file is protected from multiple pre-processor inclusion through use of the
*               BSP_STLM75 present pre-processor macro definition.
*********************************************************************************************************
*/

#ifndef  BSP_STLM75_PRESENT
#define  BSP_STLM75_PRESENT


/*
*********************************************************************************************************
*                                              EXTERNS
*********************************************************************************************************
*/

#ifdef   BSP_STLM75_MODULE
#define  BSP_STLM75_EXT
#else
#define  BSP_STLM75  extern
#endif


/*
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*/

                                                                /* -------------- STLM75 MODE DEFINES --------------- */
#define  BSP_STLM75_MODE_INTERRUPT                0             /* Interrupt Mode                                     */
#define  BSP_STLM75_MODE_COMPARATOR               1             /* Comparator Mode                                    */

                                                                /* --------------- INTERRUPT POLARITY --------------- */
#define  BSP_STLM75_INT_POL_LOW                   0             /* Output Polarity Low                                */
#define  BSP_STLM75_INT_POL_HIGH                  1             /* Output Polarity High                               */

                                                                /* --------------- FAULT TOLERANCE LEVEL ------------ */
#define  BSP_STLM75_FAULT_LEVEL_1                 0             /* 1 Consecutive Faults                               */
#define  BSP_STLM75_FAULT_LEVEL_2                 1             /* 2 Consecutive Faults                               */
#define  BSP_STLM75_FAULT_LEVEL_4                 2             /* 4 Consecutive Faults                               */
#define  BSP_STLM75_FAULT_LEVEL_6                 3             /* 6 Consecutive Faults                               */

                                                                /* ------------------ TEMPERATURE UNITS ------------- */
#define  BSP_STLM75_TEMP_UNIT_CELSIUS             0             
#define  BSP_STLM75_TEMP_UNIT_FAHRENHEIT          1
#define  BSP_STLM75_TEMP_UNIT_KELVIN              2

#define  BSP_STLM75_I2C_MAX_FREQ             400000


/*
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                               MACRO'S
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                              DATA TYPES
*********************************************************************************************************
*/

typedef  struct  bsp_stlm75_cfg {
    CPU_INT08U     FaultLevel;
    CPU_INT16S     HystTemp;
    CPU_BOOLEAN    IntPol;
    CPU_BOOLEAN    Mode;    
    CPU_INT16S     OverLimitTemp;    
    CPU_FNCT_VOID  AlarmCallBackFnct;
} BSP_STLM75_CFG;


/*
*********************************************************************************************************
*                                            FUNCTION PROTOTYPES
*********************************************************************************************************
*/

CPU_BOOLEAN      BSP_STLM75_Init         (void);
CPU_BOOLEAN      BSP_STLM75_CfgSet       (BSP_STLM75_CFG   *p_stlm75_cfg);
CPU_BOOLEAN      BSP_STLM75_TempGet      (CPU_INT08U        units,
                                          CPU_INT16S       *p_temp);

                                                                /* Not Implemented yet                                */
#if 0
BSP_STLM75_CFG   BSP_STLM75_CfgGet       (void);

CPU_BOOLEAN      BSP_STLM75_ShutDownEn   (void);
CPU_BOOLEAN      BSP_STLM75_ShutDownDis  (void);
#endif


/*
*********************************************************************************************************
*                                              MODULE END
*********************************************************************************************************
*/


#endif
