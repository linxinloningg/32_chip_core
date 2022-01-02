/*
*********************************************************************************************************
*                                     MICRIUM BOARD SUPPORT SUPPORT
*
*                          (c) Copyright 2003-2009; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*               Knowledge of the source code may NOT be used to develop a similar product.
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                     MICIUM BOARD SUPPORT PACKAGE
*                                        I2C DRIVER (MASTER ONLY)
*                                                                         
*
* Filename      : bsp_i2c.c
* Version       : V1.00
* Programmer(s) : FT
*********************************************************************************************************
* Note(s)       :
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#define  BSP_I2C_MODULE
#include <bsp.h>


/*
*********************************************************************************************************
*                                              LOCAL DEFINES
*********************************************************************************************************
*/

                                                                /* ------------- I2C BASE ADDRESS DEFINES ------------ */
#define  BSP_I2C_REG_I2C1_BASE_ADDR       (CPU_INT32U)(0x40005400)
#define  BSP_I2C_REG_I2C2_BASE_ADDR       (CPU_INT32U)(0x40005800)

                                                                /* -------- I2C CONTROL REGISTER 1 BIT DEFINES  ------- */
#define  BSP_I2C_REG_CR1_PE               DEF_BIT_00            /* Peripheral Enable                                    */
#define  BSP_I2C_REG_CR1_SMBUS            DEF_BIT_01            /* SMBUS Mode                                           */
#define  BSP_I2C_REG_CR1_SMBTYPE          DEF_BIT_03            /* SMBUS Type                                           */
#define  BSP_I2C_REG_CR1_ENARP            DEF_BIT_04            /* ARP Enable                                           */
#define  BSP_I2C_REG_CR1_ENPEC            DEF_BIT_05            /* PEC Enable                                           */
#define  BSP_I2C_REG_CR1_ENGC             DEF_BIT_06            /* ENGC General Call enable                             */
#define  BSP_I2C_REG_CR1_NOSTRETCH        DEF_BIT_07            /* Clock stretching enable                              */
#define  BSP_I2C_REG_CR1_START            DEF_BIT_08            /* Start Generation                                     */
#define  BSP_I2C_REG_CR1_STOP             DEF_BIT_09            /* Stop  Generation                                     */
#define  BSP_I2C_REG_CR1_ACK              DEF_BIT_10            /* Acknowledge Enable                                   */
#define  BSP_I2C_REG_CR1_POS              DEF_BIT_11            /* Acknowledge/PEC position (for data reception)        */
#define  BSP_I2C_REG_CR1_PEC              DEF_BIT_12            /* Packet Error Checking                                */
#define  BSP_I2C_REG_CR1_ALERT            DEF_BIT_13            /* SMBUS Aler                                           */
#define  BSP_I2C_REG_CR1_SWRST            DEF_BIT_15            /* Software Reset                                       */

                                                                /* -------- I2C CONTROL REGISTER 2 BIT DEFINES  ------- */
#define  BSP_I2C_REG_CR2_LAST             DEF_BIT_12            /* Last DMA transfer                                    */
#define  BSP_I2C_REG_CR2_DMAEN            DEF_BIT_11            /* DMA Request enable                                   */
#define  BSP_I2C_REG_CR2_ITBUFEN          DEF_BIT_10            /* Buffer interrupt enable                              */
#define  BSP_I2C_REG_CR2_ITEVTEN          DEF_BIT_09            /* Event  interrupt enable                              */
#define  BSP_I2C_REG_CR2_ITERREN          DEF_BIT_08            /* Error Interrupt enable                               */
#define  BSP_I2C_REG_CR2_FREQ_MASK        DEF_BIT_FIELD(6, 0)   /* Peripheral Clock Frequency Mask                      */

#define  BSP_I2C_REG_DR_MASK              DEF_BIT_FIELD(8, 0)   /* 8-bit Data register mask                             */

                                                                /* --------- I2C STATUS REGISTER 1 BIT DEFINES -------- */
#define  BSP_I2C_REG_SR1_ALERT            DEF_BIT_15            /* SMBUS Alert                                          */
#define  BSP_I2C_REG_SR1_TIMEOUT          DEF_BIT_14            /* Timeout error                                        */
#define  BSP_I2C_REG_SR1_PECERR           DEF_BIT_12            /* PEC error in reception                               */
#define  BSP_I2C_REG_SR1_OVR              DEF_BIT_11            /* Overrun/Underrun                                     */
#define  BSP_I2C_REG_SR1_AF               DEF_BIT_10            /* Aknowledge failure                                   */
#define  BSP_I2C_REG_SR1_ARLO             DEF_BIT_09            /* Arbitration Lost                                     */
#define  BSP_I2C_REG_SR1_BERR             DEF_BIT_08            /* Bus Error                                            */
#define  BSP_I2C_REG_SR1_TXE              DEF_BIT_07            /* Data Register empty (Transmitters)                   */
#define  BSP_I2C_REG_SR1_RXNE             DEF_BIT_06            /* Data register not empty                              */
#define  BSP_I2C_REG_SR1_STOPF            DEF_BIT_04            /* Stop detection                                       */
#define  BSP_I2C_REG_SR1_ADD10            DEF_BIT_03            /* 10-bit header sent (Master Mode)                     */
#define  BSP_I2C_REG_SR1_BTF              DEF_BIT_02            /* Byte Transfer finished                               */
#define  BSP_I2C_REG_SR1_ADDR             DEF_BIT_01            /* Address Sent                                         */
#define  BSP_I2C_REG_SR1_SB               DEF_BIT_00            /* Start bit (Mode)                                     */

                                                                /* Event Mask                                           */
#define  BSP_I2C_REG_SR1_EVENT_MASK      (BSP_I2C_REG_SR1_SB      | \
                                          BSP_I2C_REG_SR1_ADDR    | \
                                          BSP_I2C_REG_SR1_BTF     | \
                                          BSP_I2C_REG_SR1_ADDR    | \
                                          BSP_I2C_REG_SR1_RXNE    | \
                                          BSP_I2C_REG_SR1_TXE )

                                                                /* Error Mask                                           */
#define  BSP_I2C_REG_SR1_ERR_MASK        (BSP_I2C_REG_SR1_BERR    | \
                                          BSP_I2C_REG_SR1_ARLO    | \
                                          BSP_I2C_REG_SR1_AF      | \
                                          BSP_I2C_REG_SR1_OVR     | \
                                          BSP_I2C_REG_SR1_PECERR  | \
                                          BSP_I2C_REG_SR1_ALERT)

                                                                /* --------- I2C STATUS REGISTER 2 BIT DEFINES -------- */
#define  BSP_I2C_REG_SR2_PEC_MASK         DEF_BIT_FIELD(8, 8)   /* Packet error cheking register mask                   */
#define  BSP_I2C_REG_SR2_DUALF            DEF_BIT_07            /* Dual Flag (Slave mode)                               */
#define  BSP_I2C_REG_SR2_SMBHOST          DEF_BIT_06            /* SMBus Host header (Slave Mode)                       */
#define  BSP_I2C_REG_SR2_SMBDEFAULT       DEF_BIT_05            /* SMBus Device default Address                         */
#define  BSP_I2C_REG_SR2_GENCALL          DEF_BIT_04            /* General Call Address (Slave Mode)                    */
#define  BSP_I2C_REG_SR2_TRA              DEF_BIT_02            /* Trnasmitter/Receiver bit                             */
#define  BSP_I2C_REG_SR2_BUSY             DEF_BIT_01            /* Bus Busy                                             */
#define  BSP_I2C_REG_SR2_MSL              DEF_BIT_00            /* Master/Slave bit                                     */

                                                                /* ------ I2C CLOCK CONTROL REGISTER BIT DEFINES ------ */
#define  BSP_I2C_REG_CCR_FS               DEF_BIT_15            /* I2C Master Mode Selection (Standard/Fast)            */
#define  BSP_I2C_REG_CCR_DUTY             DEF_BIT_14            /* Fast Mode Duty Cycle                                 */
#define  BSP_I2C_REG_CCR_MASK             DEF_BIT_FIELD(12, 0)  /* Clock Divider                                        */


                                                                /* --------------- I2C DRIVER STATES DEFINES ---------- */
#define  BSP_I2C_STATE_IDLE                        0
#define  BSP_I2C_STATE_START                       1
#define  BSP_I2C_STATE_ADDR                        2
#define  BSP_I2C_STATE_DATA                        3
#define  BSP_I2C_STATE_STOP                        4

                                                                /* -------------- I2C ACCESS TYPE DEFINES ------------- */
#define  BSP_I2C_ACCESS_TYPE_NONE                  0
#define  BSP_I2C_ACCESS_TYPE_RD                    1
#define  BSP_I2C_ACCESS_TYPE_WR                    2
#define  BSP_I2C_ACCESS_TYPE_WR_RD                 3


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/



/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*
* Note(s) :  (1) The 'BSP_I2C_DEV_STATUS' structure defines the status of the current transfer
*
*            (2) The 'BSP_I2C_REG' defines the register set for the I2C1/I2C2 peripherals.
*********************************************************************************************************
*/

typedef  struct bsp_i2c_dev_status {
    CPU_INT08U   AccessType;                                    /* Transfer Access Type RD/WR/WR_RD                     */
    CPU_INT08U   Addr;                                          /* I2C slave address                                    */
    CPU_INT08U   State;                                         /* Current transfer state                               */
    CPU_INT08U  *BufPtr;                                        /* Pointer to the transfer data area                    */
    CPU_INT16U   BufLen;                                        /* Trnasfer length                                      */
    BSP_OS_SEM   SemLock;                                       /* I2C Exclusive access sempahore                       */
    BSP_OS_SEM   SemWait;                                       /* Transfer Complete signal                             */
} BSP_I2C_DEV_STATUS;


typedef  struct  bsp_i2c_reg {
    CPU_REG32   I2C_CR1;
    CPU_REG32   I2C_CR2;    
    CPU_REG32   I2C_OAR1;
    CPU_REG32   I2C_OAR2;
    CPU_REG32   I2C_DR;
    CPU_REG32   I2C_SR1;
    CPU_REG32   I2C_SR2;
    CPU_REG32   I2C_CCR;
    CPU_REG32   I2C_TRISE;
} BSP_I2C_REG;


/*
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  BSP_I2C_DEV_STATUS     BSP_I2C_DevTbl[BSP_I2C_NBR_MAX];

/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void        BSP_I2C1_EventISR_Handler  (void);
static  void        BSP_I2C2_EventISR_Handler  (void);
static  void        BSP_I2Cx_EventISR_Handler  (CPU_INT08U  i2c_nbr);

static  void        BSP_I2C1_ErrISR_Handler    (void);
static  void        BSP_I2C2_ErrISR_Handler    (void);
static  void        BSP_I2Cx_ErrISR_Handler    (CPU_INT08U  i2c_nbr);

static CPU_BOOLEAN  BSP_I2C_StartXfer          (CPU_INT08U   i2c_nbr,
                                                CPU_INT08U   i2c_addr,
                                                CPU_INT08U   i2c_access_type,
                                                CPU_INT08U  *p_buf,
                                                CPU_INT08U   nbr_bytes);
/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
**                                         GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                        BSP_I2C_Init()
*
* Description : Initialize the I2C.
*
* Argument(s) : i2c_id     I2C peripheral ID
*                              BSP_I2C_ID_I2C1
*                              BSP_I2C_ID_I2C2
*
*               freq       I2C clock speed. It must be set to a value lower than 100 kHz (Standard Mode) or
*                          400 Khz (Fast mode)
*
* Return(s)   : DEF_OK     If the I2C peripheral was initialized
*               DEF_FAIL   If the I2C peripheral could not be initialized.
*
* Caller(s)   : Application
*
* Note(s)     : none.
*********************************************************************************************************
*/


CPU_BOOLEAN  BSP_I2C_Init (CPU_INT08U  i2c_id,
                           CPU_INT08U  i2c_mode,
                           CPU_INT32U  clk_freq)
                    
{
    CPU_BOOLEAN          err;  
    GPIO_InitTypeDef     gpio_init_cfg;
    RCC_ClocksTypeDef    rcc_clocks;
    CPU_INT32U           pclk_freq;
    CPU_INT32U           reg_val;
    BSP_I2C_REG         *p_i2c_reg;
    BSP_I2C_DEV_STATUS  *p_i2c_dev_status;

    
                                                                /* ------------- ARGUMENTS CHECKING ----------------- */
    switch (i2c_id) {
        case BSP_I2C_ID_I2C1:
             p_i2c_reg        = (BSP_I2C_REG        *)BSP_I2C_REG_I2C1_BASE_ADDR;
             p_i2c_dev_status = (BSP_I2C_DEV_STATUS *)&BSP_I2C_DevTbl[0];
             
                                                                /* Configure the I2C1 GPIO                            */
             BSP_PeriphEn(BSP_PERIPH_ID_IOPB);             
             gpio_init_cfg.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7;
             gpio_init_cfg.GPIO_Speed = GPIO_Speed_50MHz;
             gpio_init_cfg.GPIO_Mode  = GPIO_Mode_AF_OD;
             GPIO_Init(GPIOB, &gpio_init_cfg);
 
             BSP_PeriphEn(BSP_PERIPH_ID_I2C1);                  /* Enable the I2C1 peripheral clock                   */
             
             break;

        case BSP_I2C_ID_I2C2:
             p_i2c_reg        = (BSP_I2C_REG        *)BSP_I2C_REG_I2C2_BASE_ADDR;
             p_i2c_dev_status = (BSP_I2C_DEV_STATUS *)&BSP_I2C_DevTbl[1];

                                                                /* Configure the I2C2 GPIO                            */
             BSP_PeriphEn(BSP_PERIPH_ID_IOPB);             
             gpio_init_cfg.GPIO_Pin   = GPIO_Pin_10 | GPIO_Pin_11;
             gpio_init_cfg.GPIO_Speed = GPIO_Speed_50MHz;
             gpio_init_cfg.GPIO_Mode  = GPIO_Mode_AF_OD;
             GPIO_Init(GPIOB, &gpio_init_cfg);
             
             BSP_PeriphEn(BSP_PERIPH_ID_I2C2);                  /* Enable the I2C1 peripheral clock                   */             
             break;
        
        default:
            return (DEF_FAIL);
    }
        
                                                                /* -------------- CREATE OS SEMAPHORES  ------------- */
    err = BSP_OS_SemCreate((BSP_OS_SEM    *)&(p_i2c_dev_status->SemWait),
                           (BSP_OS_SEM_VAL ) 0, 
                           (CPU_CHAR      *) "I2C Wait");          
     
    if (err == DEF_FAIL) {
        return (DEF_FAIL);
    }

    err = BSP_OS_SemCreate((BSP_OS_SEM    *)&(p_i2c_dev_status->SemLock),
                           (BSP_OS_SEM_VAL ) 1, 
                           (CPU_CHAR      *)"I2C Lock");        

    if (err == DEF_FAIL) {
        return (DEF_FAIL);
    }
        
                                                               /* ----------------- I2C INITIALIZATION -------------- */
    RCC_GetClocksFreq(&rcc_clocks);
    pclk_freq = rcc_clocks.PCLK1_Frequency;
    
    if (pclk_freq > BSP_I2C_PER_CLK_MAX_FREQ_HZ) {
       return (DEF_FAIL);
    }

    p_i2c_reg->I2C_CR1 = BSP_I2C_REG_CR1_SWRST;                /* Perform a software reset                            */
    p_i2c_reg->I2C_CR1 = DEF_BIT_NONE;          
    
                                                               /* Set the frequency range                             */
    p_i2c_reg->I2C_CR2 = (pclk_freq / DEF_TIME_NBR_uS_PER_SEC)
                       & BSP_I2C_REG_CR2_FREQ_MASK;          
    
                                                               /* Calculate the clock divider                         */
    switch (i2c_mode) {
        case BSP_I2C_MODE_STANDARD:
             if (clk_freq > BSP_I2C_MODE_STANDARD_MAX_FREQ_HZ) {
                 return (DEF_FAIL);
             }
             
             reg_val = (((2 * pclk_freq  + clk_freq)/ (2 * clk_freq))  / 2)
                     & BSP_I2C_REG_CCR_MASK;
             break;
             
        case BSP_I2C_MODE_FAST_1_2:             
             if (clk_freq > BSP_I2C_MODE_FAST_MAX_FREQ_HZ) {
                 return (DEF_FAIL);
             }  
             DEF_BIT_SET(reg_val, BSP_I2C_REG_CCR_FS);
             reg_val = (((2 * pclk_freq  + clk_freq)/ (2 * clk_freq))  / 3)
                     & BSP_I2C_REG_CCR_MASK;
             break;
        
        
        case BSP_I2C_MODE_FAST_16_9:
             if (clk_freq > BSP_I2C_MODE_FAST_MAX_FREQ_HZ) {
                 return (DEF_FAIL);
             }  
             reg_val = (((2 * pclk_freq) + (25 * clk_freq)) / (50 * clk_freq))
                     & BSP_I2C_REG_CCR_MASK; 
             DEF_BIT_SET(reg_val, BSP_I2C_REG_CCR_DUTY | BSP_I2C_REG_CCR_FS);
             break;                          
    }    
    
    p_i2c_reg->I2C_CCR = reg_val;
    
                                                                /* Enable interrupts in the interrupt controller      */
    switch (i2c_id) {
        case BSP_I2C_ID_I2C1:
             BSP_IntVectSet(BSP_INT_ID_I2C1_EV, BSP_I2C1_EventISR_Handler);
             BSP_IntEn(BSP_INT_ID_I2C1_EV);

             BSP_IntVectSet(BSP_INT_ID_I2C1_ER, BSP_I2C1_ErrISR_Handler);
             BSP_IntEn(BSP_INT_ID_I2C1_ER);
             
             break;

        case BSP_I2C_ID_I2C2:
             BSP_IntVectSet(BSP_INT_ID_I2C2_EV, BSP_I2C2_EventISR_Handler);
             BSP_IntEn(BSP_INT_ID_I2C2_EV);

             BSP_IntVectSet(BSP_INT_ID_I2C2_ER, BSP_I2C2_ErrISR_Handler);
             BSP_IntEn(BSP_INT_ID_I2C1_ER);
             break;
        
        default:
             break;
    }

                                                                 /* Initialize the device status                      */
    p_i2c_dev_status->Addr       = DEF_BIT_NONE;
    p_i2c_dev_status->AccessType = BSP_I2C_ACCESS_TYPE_NONE;
    p_i2c_dev_status->State      = BSP_I2C_STATE_IDLE;
    p_i2c_dev_status->BufPtr     = (CPU_INT08U *)0;
    p_i2c_dev_status->BufLen     = 0;   

    p_i2c_reg->I2C_CR1           = BSP_I2C_REG_CR1_PE;            /* Enable the I2C peripheral                        */
    
    return (DEF_OK);
    
}


/*
*********************************************************************************************************
*                                       BSP_I2C_StartXfer()
*
* Description : Initialize and Start a new transfer in the I2C bus.
*
* Argument(s) : i2c_id            I2C peripheral ID
*                                    BSP_I2C_ID_I2C1
*                                    BSP_I2C_ID_I2C2
*
*               i2c_addr           The I2C device address
*
*               i2c_acess_type     I2C Access Type
*                                      BSP_I2C_ACCESS_TYPE_RD
*                                      BSP_I2C_ACCESS_TYPE_WR
*                                      BSP_I2C_ACCESS_TYPE_WR_RD
*                               
*               p_buf              Pointer to the buffer into which the bytes will be stored.
*
*               nbr_bytes          Number of bytes to read.
*
* Return(s)   : DEF_OK            If the transfer could be initialized and started 
*               DEF_FAIL          If the transfer could no bet initialized and started
*
* Caller(s)   : BSP_I2C_Rd()
*               BSP_I2C_Wr()
*               BSP_I2C_WrRd()
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  BSP_I2C_StartXfer (CPU_INT08U   i2c_id,
                                        CPU_INT08U   i2c_addr,
                                        CPU_INT08U   i2c_access_type,
                                        CPU_INT08U  *p_buf,                    
                                        CPU_INT08U   nbr_bytes)
{   
    CPU_BOOLEAN          err;
    BSP_I2C_DEV_STATUS  *p_i2c_dev_status;
    BSP_I2C_REG         *p_i2c_reg;
   

    err = DEF_OK;
    
    switch (i2c_id) {
        case BSP_I2C_ID_I2C1:
             p_i2c_reg        = (BSP_I2C_REG        *)BSP_I2C_REG_I2C1_BASE_ADDR;
             p_i2c_dev_status = (BSP_I2C_DEV_STATUS *)&BSP_I2C_DevTbl[0];
             break;

        case BSP_I2C_ID_I2C2:
             p_i2c_reg        = (BSP_I2C_REG        *)BSP_I2C_REG_I2C2_BASE_ADDR;
             p_i2c_dev_status = (BSP_I2C_DEV_STATUS *)&BSP_I2C_DevTbl[1];
             break;
        
        default:
            return (DEF_FAIL);
    }
      
    err = BSP_OS_SemWait(&(p_i2c_dev_status->SemLock),          /* Lock the I2C peripheral                              */
                         0);    

    if (err == DEF_FAIL) {
        return (DEF_FAIL);
    }

                                                                /* Initialize the device structure                      */
    p_i2c_dev_status->Addr       = (i2c_addr);                  /* I2C Slave  address                                   */    
    p_i2c_dev_status->AccessType = i2c_access_type;             /* Set the access type                                  */
    p_i2c_dev_status->State      = BSP_I2C_STATE_START;         /* Set the START state                                  */
    p_i2c_dev_status->BufPtr     = p_buf;                       /* Set the buffer information                           */
    p_i2c_dev_status->BufLen     = nbr_bytes;  
    
    DEF_BIT_SET(p_i2c_reg->I2C_CR1, BSP_I2C_REG_CR1_START);     /* Generate the start condition                         */

    DEF_BIT_SET(p_i2c_reg->I2C_CR2, BSP_I2C_REG_CR2_ITEVTEN |   /* Enable Bus Errors and bus Events interrupts          */
                                    BSP_I2C_REG_CR2_ITERREN);
    
                                                                /* Wait until the transfer completes                    */
    err = BSP_OS_SemWait(&(p_i2c_dev_status->SemWait),
                         500);  
        
    BSP_OS_SemPost(&(p_i2c_dev_status->SemLock));               /* Release the I2C Peripheral                           */
        

    if (p_i2c_dev_status->BufLen != 0) {                        /* If the transfer is incomplete ...                    */
        err  = DEF_FAIL;                                        /* ... return an errror                                 */
    }
    
    return (err);
}
 

/*
*********************************************************************************************************
*                                        BSP_I2C_Rd()
*
* Description : Read 'n' bytes from the I2C bus.
*
* Argument(s) : i2c_nbr      I2C peripheral number
*                                BSP_I2C_ID_I2C1
*                                BSP_I2C_ID_I2C2
*
*               i2c_addr     The I2C device address
*
*               p_buf        Pointer to the buffer into which the bytes will be stored.
*
*               nbr_bytes    Number of bytes to be read.
*
* Return(s)   : DEF_OK       If all bytes were read.
*               DEF_FAIL     If all bytes could not be read.
*
* Caller(s)   : Application
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_BOOLEAN  BSP_I2C_Rd (CPU_INT08U   i2c_id,
                         CPU_INT08U   i2c_addr,
                         CPU_INT08U  *p_buf,
                         CPU_INT08U   nbr_bytes)
{
    CPU_BOOLEAN  err;
    
    
    if (p_buf == (CPU_INT08U *)0) {
        return (DEF_FAIL);
    }
    
    if (nbr_bytes < 1) {
        return (DEF_FAIL);
    }
    
    err = BSP_I2C_StartXfer(i2c_id,
                            i2c_addr,
                            BSP_I2C_ACCESS_TYPE_RD,
                            p_buf,
                            nbr_bytes);
    
    return (err);
}


/*
*********************************************************************************************************
*                                        BSP_I2C_Wr()
*
* Description : Write 'n' bytes tothe I2C bus.
*
* Argument(s) : i2c_nbr      I2C peripheral number
*                                BSP_I2C_ID_I2C1
*                                BSP_I2C_ID_I2C2
*
*               i2c_addr     The I2C device address
*
*               p_buf        Pointer to the buffer where the bytes will be transfered.
*
*               nbr_bytes    Number of bytes to be read.
*
* Return(s)   : DEF_OK       If all bytes were written
*               DEF_FAIL     If all bytes could not be written.
*
* Caller(s)   : Application
*
* Note(s)     : none.
*********************************************************************************************************
*/


CPU_BOOLEAN  BSP_I2C_Wr (CPU_INT08U   i2c_id,
                         CPU_INT08U   i2c_addr,
                         CPU_INT08U  *p_buf,
                         CPU_INT08U   nbr_bytes)
{
    CPU_BOOLEAN  err;
    
    
    if (p_buf == (CPU_INT08U *)0) {
        return (DEF_FAIL);
    }
    
    if (nbr_bytes < 1) {
        return (DEF_FAIL);
    }

     
    err = BSP_I2C_StartXfer(i2c_id,
                            i2c_addr,
                            BSP_I2C_ACCESS_TYPE_WR,
                            p_buf,
                            nbr_bytes);
    
    return (err);               
}


/*
*********************************************************************************************************
*                                        BSP_I2C_WrRd()
*
* Description : Perform a write followed by multiples/single read(s)
*
* Argument(s) : i2c_nbr      I2C peripheral number
*                                BSP_I2C_ID_I2C1
*                                BSP_I2C_ID_I2C2
*
*               i2c_addr     The I2C device address
*
*               p_buf        Pointer to the buffer where the bytes will be transfered/received.
*
*               nbr_bytes    Number of bytes to be read.
*
* Return(s)   : DEF_OK       If all bytes were read
*               DEF_FAIL     If all bytes could not be read.
*
* Caller(s)   : Application
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_BOOLEAN  BSP_I2C_WrRd (CPU_INT08U   i2c_id,
                           CPU_INT08U   i2c_addr,
                           CPU_INT08U  *p_buf,
                           CPU_INT08U   nbr_bytes)
{
    CPU_BOOLEAN  err;
    
    
    if (p_buf == (CPU_INT08U *)0) {
        return (DEF_FAIL);
    }
    
    if (nbr_bytes < 2) {
        return (DEF_FAIL);
    }
     
    
    err = BSP_I2C_StartXfer(i2c_id,
                            i2c_addr,
                            BSP_I2C_ACCESS_TYPE_WR_RD,
                            p_buf,
                            nbr_bytes);
    
    return (err);               
}


/*
*********************************************************************************************************
*                                        BSP_I2C1_EventISR_Handle()
*                                        BSP_I2C2_EventISR_Handle
*
* Description : I2C1/I2C2 ISR handlers
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : This is an ISR.
*
* Note(s)     : none.
*********************************************************************************************************
*/


static  void  BSP_I2C1_EventISR_Handler (void)
{
    BSP_I2Cx_EventISR_Handler(0);
}

static  void  BSP_I2C2_EventISR_Handler (void) 
{
    BSP_I2Cx_EventISR_Handler(1);    
}


/*
*********************************************************************************************************
*                                        BSP_I2Cx_EventISR_Handler()
*
* Description : Generic ISR events handler
*
* Argument(s) : i2c_nbr           I2C peripheral number.
*                                     0  I2C1 peripheral
*                                     1  I2C2 peripheral
*
* Return(s)   : none.
*
* Caller(s)   : BSP_I2C1_EventISR_Handler()
*               BSP_I2C2_EventISR_Handler()
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  BSP_I2Cx_EventISR_Handler (CPU_INT08U  i2c_nbr)
{
    BSP_I2C_DEV_STATUS  *p_i2c_dev_status;
    BSP_I2C_REG         *p_i2c_reg;
    CPU_INT32U           int_stat1;
    CPU_INT32U           int_stat2;

    
     switch (i2c_nbr) {
        case 0:
             p_i2c_reg = (BSP_I2C_REG *)BSP_I2C_REG_I2C1_BASE_ADDR;
             break;

        case 1:
             p_i2c_reg = (BSP_I2C_REG *)BSP_I2C_REG_I2C2_BASE_ADDR;
             break;
        
        default:
             break;
    }


    p_i2c_dev_status  = (BSP_I2C_DEV_STATUS *)&BSP_I2C_DevTbl[i2c_nbr];
    int_stat1         =  p_i2c_reg->I2C_SR1;
    int_stat1        &=  BSP_I2C_REG_SR1_EVENT_MASK;

    switch (p_i2c_dev_status->State) {
        case BSP_I2C_STATE_START:                               /* --------------- I2C START STATE ------------------ */
                                                                /* If the start bit flag has been generated ...       */
             if (DEF_BIT_IS_SET(int_stat1, BSP_I2C_REG_SR1_SB)) {     
                                                                /* Send the Address with the correct direction        */
                 if (p_i2c_dev_status->AccessType == BSP_I2C_ACCESS_TYPE_RD) {
                     DEF_BIT_SET(p_i2c_reg->I2C_CR1, BSP_I2C_REG_CR1_ACK);
                     p_i2c_reg->I2C_DR = (p_i2c_dev_status->Addr << 1)
                                       | DEF_BIT_00;
                 } else {                 
                     p_i2c_reg->I2C_DR = (p_i2c_dev_status->Addr << 1) & DEF_BIT_FIELD(7, 1);
                 }
                 
                 p_i2c_dev_status->State = BSP_I2C_STATE_ADDR;
             }
             break;
             
        case BSP_I2C_STATE_ADDR:                                /* --------------- I2C ADRESS STATE ----------------- */
                                                                /* If the address was sent ...                        */
             if (DEF_BIT_IS_SET(int_stat1, BSP_I2C_REG_SR1_ADDR)) { 
                 int_stat2 = p_i2c_reg->I2C_SR2;
              
                 (void)&int_stat2;

                 switch (p_i2c_dev_status->AccessType) {
                     case BSP_I2C_ACCESS_TYPE_RD:
                          if (p_i2c_dev_status->BufLen == 1) {
                              p_i2c_dev_status->State = BSP_I2C_STATE_STOP;                               
                              DEF_BIT_CLR(p_i2c_reg->I2C_CR1, BSP_I2C_REG_CR1_ACK);
                          } else {                              
                              p_i2c_dev_status->State = BSP_I2C_STATE_DATA;
                          }
                          break;                 

                     case BSP_I2C_ACCESS_TYPE_WR:
                     case BSP_I2C_ACCESS_TYPE_WR_RD:
                          if (DEF_BIT_IS_SET(int_stat1, BSP_I2C_REG_SR1_TXE)) {                     
                              p_i2c_reg->I2C_DR = (CPU_INT32U)(*(p_i2c_dev_status->BufPtr));
                              p_i2c_dev_status->BufPtr++;
                              p_i2c_dev_status->BufLen--;
                              if (p_i2c_dev_status->BufLen == 0) {                               
                                  p_i2c_dev_status->State = BSP_I2C_STATE_STOP;                               
                              } else {
                                  p_i2c_dev_status->State = BSP_I2C_STATE_DATA;
                              }
                          }
                          break;  

                     default:
                          break;
                 }             
             } else {
                 p_i2c_dev_status->State      = BSP_I2C_STATE_IDLE;
                 p_i2c_dev_status->AccessType = BSP_I2C_ACCESS_TYPE_NONE;

                 DEF_BIT_CLR(p_i2c_reg->I2C_CR2, BSP_I2C_REG_CR2_ITEVTEN |
                                                 BSP_I2C_REG_CR2_ITERREN);
                 DEF_BIT_SET(p_i2c_reg->I2C_CR1, BSP_I2C_REG_CR1_STOP);                              

                 BSP_OS_SemPost(&(p_i2c_dev_status->SemWait));
             }
             break;
                 
         case BSP_I2C_STATE_DATA:                               /* ---------------- I2C DATA STATE ------------------ */
              switch (p_i2c_dev_status->AccessType) {
                                                                /* If the I2C is receiving ...                        */
                  case BSP_I2C_ACCESS_TYPE_WR_RD:  
                       if (DEF_BIT_IS_SET(int_stat1, BSP_I2C_REG_SR1_TXE)) {
                                                                /* Initialize the Transfer as read access             */
                           DEF_BIT_SET(p_i2c_reg->I2C_CR1, BSP_I2C_REG_CR1_START);
                           p_i2c_dev_status->State      = BSP_I2C_STATE_START;
                           p_i2c_dev_status->AccessType = BSP_I2C_ACCESS_TYPE_RD;
                       }
                       break;

                  case BSP_I2C_ACCESS_TYPE_RD:  
                                                                /* If the receive register is not empty               */
                       if (DEF_BIT_IS_SET(int_stat1, BSP_I2C_REG_SR1_RXNE)) {
                           *(p_i2c_dev_status->BufPtr) = (CPU_INT08U)(p_i2c_reg->I2C_DR & BSP_I2C_REG_DR_MASK);
                             p_i2c_dev_status->BufPtr++;
                             p_i2c_dev_status->BufLen--;
                                                               /*  If it is the last byte                             */
                           if (p_i2c_dev_status->BufLen == 1) {                               
                                                               /*  NOT Acknowledge, Generate STOP condition           */
                               DEF_BIT_CLR(p_i2c_reg->I2C_CR1, BSP_I2C_REG_CR1_ACK);
                               DEF_BIT_SET(p_i2c_reg->I2C_CR1, BSP_I2C_REG_CR1_STOP);                              
                               p_i2c_dev_status->State = BSP_I2C_STATE_STOP;                               
                           }
                       }                        
                       break;                 
                  
                  case BSP_I2C_ACCESS_TYPE_WR:
                       if (DEF_BIT_IS_SET(int_stat1, BSP_I2C_REG_SR1_TXE)) {
                           p_i2c_reg->I2C_DR = (CPU_INT32U)(*(p_i2c_dev_status->BufPtr));
                           p_i2c_dev_status->BufPtr++;
                           p_i2c_dev_status->BufLen--;
                           if (p_i2c_dev_status->BufLen == 0) {                               
                               p_i2c_dev_status->State = BSP_I2C_STATE_STOP;                               
                           }
                       }
                       break;       
              }
              break;
        
        case BSP_I2C_STATE_STOP:                                /* ---------------- I2C STOP STATE ------------------ */
             if (DEF_BIT_IS_SET(int_stat1, BSP_I2C_REG_SR1_BTF)) {     
                 switch (p_i2c_dev_status->AccessType) {
                     case BSP_I2C_ACCESS_TYPE_WR_RD:
                     case BSP_I2C_ACCESS_TYPE_RD:  
                          DEF_BIT_SET(p_i2c_reg->I2C_CR1, BSP_I2C_REG_CR1_STOP);                                   
                          *(p_i2c_dev_status->BufPtr) = (CPU_INT08U)(p_i2c_reg->I2C_DR & BSP_I2C_REG_DR_MASK);;
                            p_i2c_dev_status->BufPtr++;
                            p_i2c_dev_status->BufLen--;
                          break;
                    
                     case BSP_I2C_ACCESS_TYPE_WR:
                          DEF_BIT_CLR(p_i2c_reg->I2C_CR1, BSP_I2C_REG_CR1_ACK);
                          DEF_BIT_SET(p_i2c_reg->I2C_CR1, BSP_I2C_REG_CR1_STOP);                              
                          break;
                 }
                 p_i2c_dev_status->State      = BSP_I2C_STATE_IDLE;
                 p_i2c_dev_status->AccessType = BSP_I2C_ACCESS_TYPE_NONE;

                 DEF_BIT_CLR(p_i2c_reg->I2C_CR2, BSP_I2C_REG_CR2_ITEVTEN |
                                                 BSP_I2C_REG_CR2_ITERREN);
                 
                 BSP_OS_SemPost(&(p_i2c_dev_status->SemWait));
             }
             break;

        case BSP_I2C_STATE_IDLE:
        default:
             break;
    }

}


/*
*********************************************************************************************************
*                                        BSP_I2Cx_EventISR_Handler()
*
* Description : Generic ISR events handler
*
* Argument(s) : i2c_nbr           I2C peripheral number.
*                                     0  I2C1 peripheral
*                                     1  I2C2 peripheral
*
* Return(s)   : none.
*
* Caller(s)   : BSP_I2C1_EventISR_Handler()
*               BSP_I2C2_EventISR_Handler()
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  BSP_I2C1_ErrISR_Handler    (void) 
{
    BSP_I2Cx_ErrISR_Handler(0);
}


static  void  BSP_I2C2_ErrISR_Handler    (void)
{
    BSP_I2Cx_ErrISR_Handler(1);
}


/*
*********************************************************************************************************
*                                        BSP_I2Cx_ErrISR_Handler()
*
* Description : Generic ISR errors handler
*
* Argument(s) : i2c_nbr           I2C peripheral number.
*                                     0  I2C1 peripheral
*                                     1  I2C2 peripheral
*
* Return(s)   : none.
*
* Caller(s)   : BSP_I2C1_ErrISR_Handler()
*               BSP_I2C2_ErrISR_Handler()
*
* Note(s)     : none
*********************************************************************************************************
*/

static  void  BSP_I2Cx_ErrISR_Handler (CPU_INT08U  i2c_nbr)
{
    BSP_I2C_DEV_STATUS  *p_i2c_dev_status;
    BSP_I2C_REG         *p_i2c_reg;
    CPU_INT32U           int_stat1;

    
     switch (i2c_nbr) {
        case 0:
             p_i2c_reg = (BSP_I2C_REG *)BSP_I2C_REG_I2C1_BASE_ADDR;
             break;

        case 1:
             p_i2c_reg = (BSP_I2C_REG *)BSP_I2C_REG_I2C2_BASE_ADDR;
             break;
        
        default:
             break;
    }

    p_i2c_dev_status  = (BSP_I2C_DEV_STATUS *)&BSP_I2C_DevTbl[i2c_nbr];
    int_stat1         =  p_i2c_reg->I2C_SR1;
    int_stat1        &=  BSP_I2C_REG_SR1_ERR_MASK;
    
    DEF_BIT_CLR(p_i2c_reg->I2C_SR1, int_stat1);
    
    if (p_i2c_dev_status->State != BSP_I2C_STATE_IDLE) {
        p_i2c_dev_status->State      = BSP_I2C_STATE_IDLE;
        p_i2c_dev_status->AccessType = BSP_I2C_ACCESS_TYPE_NONE;
        DEF_BIT_SET(p_i2c_reg->I2C_CR1, BSP_I2C_REG_CR1_STOP);                              

        DEF_BIT_CLR(p_i2c_reg->I2C_CR2, (BSP_I2C_REG_CR2_ITEVTEN |
                                         BSP_I2C_REG_CR2_ITERREN));
                 
        BSP_OS_SemPost(&(p_i2c_dev_status->SemWait));
    }
}
