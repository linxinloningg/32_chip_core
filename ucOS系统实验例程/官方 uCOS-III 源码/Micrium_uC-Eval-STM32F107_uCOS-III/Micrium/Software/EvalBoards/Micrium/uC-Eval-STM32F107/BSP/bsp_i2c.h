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
*                                        MICIUM BOARD SUPPORT PACKAGE
*                                               I2C INTERFACE
*
* Filename      : bsp_i2c.h
* Version       : V1.00
* Programmer(s) : FT
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                                 MODULE
*
* Note(s) : (1) This header file is protected from multiple pre-processor inclusion through use of the
*               BSP_I2C present pre-processor macro definition.
*********************************************************************************************************
*/

#ifndef  BSP_I2C_PRESENT
#define  BSP_I2C_PRESENT


/*
*********************************************************************************************************
*                                              EXTERNS
*********************************************************************************************************
*/

#ifdef   BSP_I2C_MODULE
#define  BSP_I2C_EXT
#else
#define  BSP_I2C  extern
#endif


/*
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*/

#define  BSP_I2C_ID_I2C1                              1
#define  BSP_I2C_ID_I2C2                              2

#define  BSP_I2C_NBR_MAX                              2

#define  BSP_I2C_PER_CLK_MAX_FREQ_HZ           36000000



#define  BSP_I2C_MODE_STANDARD_MAX_FREQ_HZ       100000
#define  BSP_I2C_MODE_STANDARD                        0

#define  BSP_I2C_MODE_FAST_MAX_FREQ_HZ           400000
#define  BSP_I2C_MODE_FAST_1_2                        1
#define  BSP_I2C_MODE_FAST_16_9                       2

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


/*
*********************************************************************************************************
*                                            FUNCTION PROTOTYPES
*********************************************************************************************************
*/

CPU_BOOLEAN  BSP_I2C_Init   (CPU_INT08U  i2c_id,
                             CPU_INT08U  i2c_mode,
                             CPU_INT32U  clk_freq);

CPU_BOOLEAN  BSP_I2C_Wr    (CPU_INT08U   i2c_nbr,
                            CPU_INT08U   i2c_addr,
                            CPU_INT08U  *p_buf,                    
                            CPU_INT08U   nbr_bytes);
                     
CPU_BOOLEAN  BSP_I2C_Rd    (CPU_INT08U   i2c_nbr,
                            CPU_INT08U   i2c_addr,
                            CPU_INT08U  *p_buf,
                            CPU_INT08U   nbr_bytes);

CPU_BOOLEAN  BSP_I2C_WrRd  (CPU_INT08U   i2c_nbr,
                            CPU_INT08U   i2c_addr,
                            CPU_INT08U  *p_buf,
                            CPU_INT08U   nbr_bytes);


/*
*********************************************************************************************************
*                                              MODULE END
*********************************************************************************************************
*/


#endif
