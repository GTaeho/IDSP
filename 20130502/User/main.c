/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "stm32f4xx_it.h"
#include "wiz820io.h"
#include "config.h"
#include "socket.h"
#include "w5200.h"
#include "tlcd.h"
#include "rtc.h"
#include "glcd.h"
#include "exti.h"
#include "myAccel3LV02.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define SELECT_AXIS_X   0
#define SELECT_AXIS_Y   1
#define SELECT_AXIS_Z   2

/* Private enumeration --------------------------------------------------------*/

/* Text Parser delimiter */
enum { EQ_ONE, EQ_TWO, PC_Cli };

/* Private variables ---------------------------------------------------------*/
// Timer variables
uint16_t PrescalerValue = 0;
extern volatile uint16_t TimerCount;

// UART4 Messages buffer
extern volatile uint8_t RxBuffer[RxBufferSize];
extern volatile uint8_t RxCounter;
extern volatile uint8_t ParseUART4;

// RCC Check variable
volatile uint32_t SYSCLK_Freq;
volatile uint32_t HCLK_Freq;
volatile uint32_t PCLK1_Freq;
volatile uint32_t PCLK2_Freq;

// GLCD Graph container from stm32f4xx_it.c
extern unsigned char mode, i, j, x, y, offset_start, offset_finish;
extern int BitCount, index, RbitFlag, flag_uart, tmp_start;

// GAL container
double tmp_gal = 0;
unsigned short max_gal = 0;

// String parse flags from wiz820io.c
extern uint8_t E1Flag;
extern uint8_t E2Flag;
extern uint8_t PCFlag;

// WIZ820io TX, RX Buffer
extern uint8 TX_BUF[TX_RX_MAX_BUF_SIZE]; // TX Buffer for applications
extern uint8 RX_BUF[TX_RX_MAX_BUF_SIZE]; // RX Buffer for applications

// AxisData Buffer typedef struct from stm32f4xx_it.h
extern AXISDATA mAxisData;
extern AXISBUF  mAxisBuf;

// GAL Buffer originally called here
GALBUF   mGalBuf;

// create 10 second enough data to retain from config.h
EQ_DAQ_ONE DAQBoardOne[1000];
EQ_DAQ_ONE DAQBoardTwo[1000];

// myAccel3LV02 Axis data
uint16_t Xdata, Ydata, Zdata;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Configures the different system clocks.
  * @param  None
  * @retval None
  */
void RCC_Configuration(void) {
  RCC_ClocksTypeDef RCC_ClockFreq;
  
  RCC_GetClocksFreq(&RCC_ClockFreq);
  
  SYSCLK_Freq = RCC_ClockFreq.SYSCLK_Frequency;
  HCLK_Freq = RCC_ClockFreq.HCLK_Frequency;
  PCLK1_Freq = RCC_ClockFreq.PCLK1_Frequency;
  PCLK2_Freq = RCC_ClockFreq.PCLK2_Frequency;
}

void TIM4_Configuration(void) {
  /* TIM4 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
  
  /* -----------------------------------------------------------------------
    TIM4 Configuration: Output Compare Timing Mode:
    
    In this example TIM4 input clock (TIM4CLK) is set to 2 * APB1 clock (PCLK1), 
    since APB1 prescaler is different from 1.   
      TIM4CLK = 2 * PCLK1
      PCLK1 = HCLK / 4
      => TIM4CLK = HCLK / 2 = SystemCoreClock /2
	  (HCLK = 168MHz, PCLK1 = 42MHz, TIM4LCK = 84MHz)
          
    To get TIM4 counter clock at 84 KHz, the prescaler is computed as follows:
       Prescaler = (TIM4CLK / TIM4 counter clock) - 1
       Prescaler = ((SystemCoreClock /2) /84 KHz) - 1
                                              
    CC1 update rate = TIM4 counter clock / CCR1_Val = 5 Hz
    ==> Toggling frequency = 2.5 Hz

    Note: 
     SystemCoreClock variable holds HCLK frequency and is defined in system_stm32f4xx.c file.
     Each time the core clock (HCLK) changes, user had to call SystemCoreClockUpdate()
     function to update SystemCoreClock variable value. Otherwise, any configuration
     based on this variable will be incorrect.
  ----------------------------------------------------------------------- */  
    
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

  /* Compute the prescaler value */
  PrescalerValue = (uint16_t) ((SystemCoreClock / 2) / 84) - 1;

  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 1000;
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

  /* Prescaler configuration */
  TIM_PrescalerConfig(TIM4, 83, TIM_PSCReloadMode_Immediate);

  /* TIM Interrupts enable */
  TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);

  /* TIM4 enable counter */
  TIM_Cmd(TIM4, ENABLE);
  
  /* NVIC Configuration -----------------------------------*/
  NVIC_InitTypeDef NVIC_InitStructure;

  /* Enable the TIM4 gloabal Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

void UART4_Configuration(void) {
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  
  /* RCC configuration -------------------------------------------------------*/
  /* Enable GPIOA for UART4 clock*/
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  
  /* USART4 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
  
  /* GPIO configuration ------------------------------------------------------*/
  /* configure AF */
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource0, GPIO_AF_UART4);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_UART4);
    
  /* Configure UART4 Tx as alternate function */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* Configure UART4 Rx as input mode */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* UART4 configuration -----------------------------------------------------*/
  USART_InitStructure.USART_BaudRate = 9600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  /* Configure UART4 */
  USART_Init(UART4, &USART_InitStructure);
 
  /* Enable the UART4 Receive interrupt: this interrupt is generated when the
     UART4 receive data register is not empty */
  USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);
  
  /* Enable the UART4 */
  USART_Cmd(UART4, ENABLE);
  
  /* NVIC Configuration -----------------------------------*/
  NVIC_InitTypeDef NVIC_InitStructure;
  
  /* Enable the UART4 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

/* RETARGET PRINTF */
int fputc(int ch, FILE *stream) {
  while(USART_GetFlagStatus(UART4, USART_FLAG_TXE) == RESET);
  
  USART_SendData(UART4, (char) ch);

  return ch;
}

void ProcessTextStream(uint8_t which, char *string) {
  char *TokenOne = NULL;
  char *TokenTwo = NULL;
  
  switch(which) {
  case EQ_ONE : 
    TokenOne = strtok(string, "_"); // result contains Date. ex) 20130428
    DAQBoardOne[0].Date = TokenOne;
    TokenOne = strtok(NULL, "_"); // result contains Time. ex) 21384729
    DAQBoardOne[0].Time = TokenOne;
    TokenOne = strtok(NULL, "_"); // result contains Axis X Data. ex) +4096
    DAQBoardOne[0].AxisX = TokenOne;
    TokenOne = strtok(NULL, "_"); // result contains Axis Y Data. ex) +4096
    DAQBoardOne[0].AxisY = TokenOne;
    TokenOne = strtok(NULL, "_"); // result contains Axis Z Data. ex) +4096
    DAQBoardOne[0].AxisZ = TokenOne;
    
    // DEBUG
    printf("\r\n--------------------------------");
    printf("\r\nEQ_ONE, Date : %s", DAQBoardOne[0].Date);
    printf("\r\nEQ_ONE, Time : %s", DAQBoardOne[0].Time);
    printf("\r\nEQ_ONE, AxisX : %s", DAQBoardOne[0].AxisX);
    printf("\r\nEQ_ONE, AxisY : %s", DAQBoardOne[0].AxisY);
    printf("\r\nEQ_ONE, AxisZ : %s", DAQBoardOne[0].AxisZ);
    break;
    
  case EQ_TWO :
    TokenTwo = strtok(string, "_"); // result contains Date. ex) 20130428
    DAQBoardTwo[0].Date = TokenTwo;
    TokenTwo = strtok(NULL, "_"); // result contains Time. ex) 21384729
    DAQBoardTwo[0].Time = TokenTwo;
    TokenTwo = strtok(NULL, "_"); // result contains Axis X Data. ex) +4096
    DAQBoardTwo[0].AxisX = TokenTwo;
    TokenTwo = strtok(NULL, "_"); // result contains Axis Y Data. ex) +4096
    DAQBoardTwo[0].AxisY = TokenTwo;
    TokenTwo = strtok(NULL, "_"); // result contains Axis Z Data. ex) +4096
    DAQBoardTwo[0].AxisZ = TokenTwo;
    break;
  case PC_Cli: break;
  default: break;
  }
}

// Copy untrimmed data to GAL array with its crude data trimmed
void CopyToTmpGalArray(uint8_t index) {
  if(mAxisBuf.tmp_data_x_lcd[index] & 0x2000)
    mGalBuf.tmp_gal_x[index] = (~mAxisBuf.tmp_data_x_lcd[index] + 1) & 0x1FFF;
  else
    mGalBuf.tmp_gal_x[index] = mAxisBuf.tmp_data_x_lcd[index] & 0x1FFF;
    
  if(mAxisBuf.tmp_data_y_lcd[index] & 0x2000)
    mGalBuf.tmp_gal_y[index] = (~mAxisBuf.tmp_data_y_lcd[index] + 1) & 0x1FFF;
  else
    mGalBuf.tmp_gal_y[index] = mAxisBuf.tmp_data_y_lcd[index] & 0x1FFF;
  
  if(mAxisBuf.tmp_data_z_lcd[index] & 0x2000)
    mGalBuf.tmp_gal_z[index] = (~mAxisBuf.tmp_data_z_lcd[index] + 1) & 0x1FFF;
  else
    mGalBuf.tmp_gal_z[index] = mAxisBuf.tmp_data_z_lcd[index] & 0x1FFF;
}

// According to theory, Maximum G covers 2Gs. so we need to cut off surplus 1G
// there exist in both +, - area.
void CutOffTo1G(uint8_t index) {
  /* Axis X --------------------------------------*/
  //when it is positive value
  if(!(mAxisBuf.tmp_data_x_lcd[index] & 0x2000)) {
    if(mAxisBuf.tmp_data_x_lcd[index] & 0x1000) {
      mAxisBuf.tmp_data_x_lcd[index] = 0x0FFF;
    }
  } else { //when it is negative value
    if(!(mAxisBuf.tmp_data_x_lcd[index] & 0x1000)) {
      mAxisBuf.tmp_data_x_lcd[index] = 0x3001;
    }
  }
  
  /* Axis Y --------------------------------------*/
  if(!(mAxisBuf.tmp_data_y_lcd[index] & 0x2000)) {
    if(mAxisBuf.tmp_data_y_lcd[index] & 0x1000) {
      mAxisBuf.tmp_data_y_lcd[index] = 0x0FFF;
    }
  } else {
    if(!(mAxisBuf.tmp_data_y_lcd[index] & 0x1000)) {
      mAxisBuf.tmp_data_y_lcd[index] = 0x3001;
    }
  }
  
  /* Axis Z --------------------------------------*/
  if(!(mAxisBuf.tmp_data_z_lcd[index] & 0x2000)) {
    if(mAxisBuf.tmp_data_z_lcd[index] & 0x1000) {
      mAxisBuf.tmp_data_z_lcd[index] = 0x0FFF;
    }
  } else {
    if(!(mAxisBuf.tmp_data_z_lcd[index] & 0x1000)) {
      mAxisBuf.tmp_data_z_lcd[index] = 0x3001;
    }
  }  
}

void CalculateGalAndCopyToGal(uint8_t index) {
  // Calculate GAL
  tmp_gal = sqrt(((mGalBuf.tmp_gal_x[index]/4) * (mGalBuf.tmp_gal_x[index]/4)) + 
                 ((mGalBuf.tmp_gal_y[index]/4) * (mGalBuf.tmp_gal_y[index]/4)) + 
                 ((mGalBuf.tmp_gal_z[index]/4) * (mGalBuf.tmp_gal_z[index]/4)));
  
  // Copy to GAL Array
  mAxisData.data_g[index] = (int)tmp_gal;
}

// Determine KMA scale
void DetermineKMA(uint8_t index) {
  if(mAxisData.data_g[index+1] > mAxisData.data_g[index]) {
    max_gal = mAxisData.data_g[index+1];
  } else {
    max_gal = max_gal;
  }
}

/**
  * @brief   Main program
  * @param  None
  * @retval None
  */
int main(void) {
  /*!< At this stage the microcontroller clock setting is already configured, 
       this is done through SystemInit() function which is called from startup
       file (startup_stm32f10x_xx.s) before to branch to application main.
       To reconfigure the default setting of SystemInit() function, refer to
       system_stm32f10x.c file
     */     
       
  /* RCC configuration */
  RCC_Configuration();
  
  /* TIM4 configuration */
  TIM4_Configuration();
  
  /* UART4 configuration */
  UART4_Configuration();
  
  /* Configure SysTick for each 1us */  
  SysTick_Config(SystemCoreClock / 1000000);
  
  /* TLCD configuration */
  TLCD_Configuration();

  /* RTC Configuration */
  RTC_Configuration();
  
  /* WIZ820io SPI2 configuration */
  WIZ820io_SPI2_Configuration();  // one that is on left one
  
  /* WIZ820io SPI3 configuration */
  //WIZ820io_SPI3_Configuration();  // one that is on right one

  /* W5200 Configuration */
  Set_network();
  
  /* EXIT4(Mode select button) configuration */
  EXTILine4_Configuration();
  
  /* EXIT5(F_SYNC : 100Hz) configuration */
  EXTILine5_Configuration();
  
  /* EXIT6(F_SCLK : 10KHz) configuration */
  EXTILine6_Configuration();
  
  /* myAccel3LV02 Configuration */
  Accel3LV02_Configuration();
  
  //myAccel3LV02 setup 1000.0111 Power on, enable all axis, self test off
  Accel_WriteReg(CTRL_REG1, 0xC7);
  // following routine setup myAccel3LV02 6g mode
  //Accel_WriteReg(CTRL_REG2, 0x80);
  
  /* GLCD configuration */
  GLCD_Configuration();
  
  // When everything is set, print message
  printf("\r\n - System is ready - ");
  
  while(1) {
    if(TimerCount >= 1000) {  // thousand equals one second
      TimerCount = 0;

      /*
      // retrieve axis data
      GetAccelValue(AXIS_X, &Xdata);
      GetAccelValue(AXIS_Y, &Ydata);
      GetAccelValue(AXIS_Z, &Zdata);
      
      char str[30];
      sprintf(str, "%d,%d,%d", 0xFFF&Xdata, 0xFFF&Ydata, 0xFFF&Zdata);
      TLCD_Clear();
      TLCD_Write(0, 0, str);
      */
    }
    
    if(ParseUART4) {
      ParseUART4 = false; 
      
      // print Wiz810io configuration
      printSysCfg();
    }
    
    if(flag_uart == 1) {
      tmp_start = index;
    }
    
    // On every impulse out of 100Hz do the work
    if(RbitFlag) {
      RbitFlag = false;
      
      // copy to buffer
      mAxisBuf.tmp_data_x_lcd[index] = mAxisData.data_x[index];
      mAxisBuf.tmp_data_y_lcd[index] = mAxisData.data_y[index];
      mAxisBuf.tmp_data_z_lcd[index] = mAxisData.data_z[index];
      
      // Copy to Temporary GAL array
      CopyToTmpGalArray(index);
      
      // Cut off to 1G
      CutOffTo1G(index);
      
      // Calculate GAL and copy to single temporary GAL value
      CalculateGalAndCopyToGal(index);
      
      // Determine KMA scale
      DetermineKMA(index);
      
      /* PC Client Parsing routine ------------------------------------------------- */
      /* Set PCFlag indicate that we have valid connection from PC Client(port 7070) */
      if(PCFlag) {
        //PCFlag = false;
        
        char PC_Buf[20];
        sprintf(PC_Buf, "%+d,%+d,%+d\n",
                mAxisBuf.tmp_data_x_lcd[index],
                mAxisBuf.tmp_data_y_lcd[index],
                mAxisBuf.tmp_data_z_lcd[index]);
        // code for stacking algorithm which will combine data from two boards into one
        // Only when socket is established, allow send data
        if(getSn_SR(SOCK_TWO) == SOCK_ESTABLISHED) {
          /* send selected data */
          send(SOCK_TWO, (uint8_t*)PC_Buf, strlen(PC_Buf), (bool)false);
        }
      }
      
      // increase index so that we can add to next array
      index++;
    }
        
    switch(mode) {
    case SELECT_AXIS_X : break;
    case SELECT_AXIS_Y : break;
    case SELECT_AXIS_Z : break;
    }
    
    /* EQ-DAQ-01 Parsing routine ------------------------------------------------- */
    /* Set E1Flag indicate that we have valid connection from EQ-DAQ-01(port 5050) */
    if(E1Flag) {
      E1Flag = false;
      
      ProcessTextStream(EQ_ONE, (char*)RX_BUF);
    }
    
    /* EQ-DAQ-02 Parsing routine ------------------------------------------------- */
    /* Set E2Flag indicate that we have valid connection from EQ-DAQ-02(port 6060) */
    if(E2Flag) {
      E2Flag = false;
      
      ProcessTextStream(EQ_TWO, (char*)RX_BUF);
    }
    
    /* Process server socket with each port */
    ProcessTcpServer(SOCK_ZERO, 5050);  // designated as for EQM-DAQ-01 with port 5050
    ProcessTcpServer(SOCK_ONE, 6060);   // designated as for EQM-DAQ-02 with port 6060
    ProcessTcpServer(SOCK_TWO, 7070);   // designated as for PC-CLIENT  with port 7070
    ProcessTcpServer(SOCK_THREE, 8080); // designated as for TOBEUSED   with port 8080
    
    /* Socket 4 to 7 reserved for future application
     * ProcessTcpServer(SOCK_FOUR, 9090);   // designated as for TOBEUSED with port 9090
     * ProcessTcpServer(SOCK_FIVE, 10010);   // designated as for TOBEUSED with port 10010
     * ProcessTcpServer(SOCK_SIX, 10020);    // designated as for TOBEUSED with port 10020
     * ProcessTcpServer(SOCK_SEVEN, 10030);  // designated as for TOBEUSED with port 10030
     */
  }
}


#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1) {
    printf("assert_failed");
  }
}
#endif

