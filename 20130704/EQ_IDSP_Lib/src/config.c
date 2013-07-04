/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f10x_it.h"
#include "config.h"
#include "usb_type.h"
#include "socket.h"
#include "w5200.h"
#include "wiz820io.h"
#include "atfc.h"
#include "rtc.h"
#include "ff.h"
#include "fatfs.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Text Parser delimiter */
enum { EQ_ONE, EQ_TWO, EQ_BOTH, PC_Cli, PC_DUMP };

/* Private variables ---------------------------------------------------------*/

// RCC Check variable
volatile uint32_t SYSCLK_Freq;
volatile uint32_t HCLK_Freq;
volatile uint32_t PCLK1_Freq;
volatile uint32_t PCLK2_Freq;

// AxisData Buffer typedef struct from stm32f4xx_it.h
extern AXISDATA    mAxisData;
extern AXISBUF     mAxisBuf;
extern AXISNETBUF  mAxisNetBuf;

// GAL container
double tmp_gal = 0;
unsigned short max_gal = 0;

// GAL Buffer originally called here
GALBUF   mGalBuf;

// 10 seconds enough data to retain which from config.h
EQ_DAQ_ONE DAQBoardOne[100];
EQ_DAQ_ONE DAQBoardTwo[100];

// String Container from main.c
extern char DATA1_BUF[19500];
extern char DATA2_BUF[19500];

// Each second count
int OneSecCount = 0;

// Elapsed time in second scale
int TimeElapsedInSecond = 0;

// Every second flag
bool EachSecFlag = false;

/* FatFs elements from config.c */
extern FATFS fs;         /* Work area (file system object) for logical drive */
extern FIL fsrc;         /* file objects */
extern FRESULT res;
extern UINT br;

// Directory, File string container pointer from main.h
extern char *filePath;

// GLCD Graph container from stm32f10x_it.c
extern volatile int SyncFlag;

// AppendFatFs relevant variables
int mX, mY, mZ;

// time variables update itself by setting from main.c
extern volatile int year, month, day, hour, minute, second, tmsecond;

// KMA Intensity count
int ACCn;
int KMACount, ACCnAddition;
float MeanACC, MaxACC, MaxMeanACC;
int MaxMeanCount;
int ACCnFifty = 50;
int ACCnArr[50];

// ATFC Event Detection flag
extern volatile bool EventDetection;

// after 13 second go ahead apply ATFC algorithm from main.c
extern volatile bool GoATFCFlag;
extern volatile bool ParamCompleteFlag;

// ATFC Algorithm variable container
int mAlgorithmContainer;

// IP force setting
extern __IO uint8 IP[4];


/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Configures the different system clocks.
  * @param  None
  * @retval None
  */
void RCC_Configuration(void) {
  RCC_ClocksTypeDef RCC_ClockFreq;
  
  RCC_GetClocksFreq(&RCC_ClockFreq);
  
  /* Wait till HSE is ready */
  while (RCC_GetFlagStatus(RCC_FLAG_HSERDY) == RESET);
  
  SYSCLK_Freq = RCC_ClockFreq.SYSCLK_Frequency;
  HCLK_Freq = RCC_ClockFreq.HCLK_Frequency;
  PCLK1_Freq = RCC_ClockFreq.PCLK1_Frequency;
  PCLK2_Freq = RCC_ClockFreq.PCLK2_Frequency;  
}

/**
  * @brief  Configures TIM2 clock and interrupt.
  * @param  None
  * @retval None
  */
void TIM2_Configuration(void) {
  NVIC_InitTypeDef NVIC_InitStructure;
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  
  /* Prescaler value */
  uint16_t PrescalerValue = 0;
  
  /* Enable TIM2 Clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
  
  /* Enable the TIM2 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Compute the prescaler value */
  PrescalerValue = (uint16_t) (SystemCoreClock / 1000000) - 1;

  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 1000;
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

  /* Prescaler configuration */
  TIM_PrescalerConfig(TIM2, PrescalerValue, TIM_PSCReloadMode_Immediate);

  /* TIM IT enable */
  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

  /* TIM2 enable counter */
  TIM_Cmd(TIM2, ENABLE); 
}

/**
  * @brief  Configures TIM3 clock and interrupt dedicate to server.
  * @param  None
  * @retval None
  */
void TIM3_Configuration(void) {
  NVIC_InitTypeDef NVIC_InitStructure;
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  
  /* Prescaler value */
  uint16_t PrescalerValue = 0;
  
  /* Enable TIM3 Clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
  
  /* Enable the TIM3 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Compute the prescaler value */
  PrescalerValue = (uint16_t) (SystemCoreClock / 1000000) - 1;

  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 1000;
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

  /* Prescaler configuration */
  TIM_PrescalerConfig(TIM3, PrescalerValue, TIM_PSCReloadMode_Immediate);

  /* TIM IT enable */
  TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

  /* TIM3 enable counter */
  TIM_Cmd(TIM3, ENABLE); 
}

/**
  * @brief  Configures TIM4 clock and interrupt.
  * @param  None
  * @retval None
  */
void TIM4_Configuration(void) {
  NVIC_InitTypeDef NVIC_InitStructure;
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  
  /* Prescaler value */
  uint16_t PrescalerValue = 0;
  
  /* Enable TIM4 Clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
  
  /* Enable the TIM4 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 4;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Compute the prescaler value */
  PrescalerValue = (uint16_t) (SystemCoreClock / 1000000) - 1;

  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 1000;
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

  /* Prescaler configuration */
  TIM_PrescalerConfig(TIM4, PrescalerValue, TIM_PSCReloadMode_Immediate);

  /* TIM IT enable */
  TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);

  /* TIM4 enable counter */
  TIM_Cmd(TIM4, ENABLE); 
}

/**
  * @brief  Configures TIM5 clock and interrupt.
  * @param  None
  * @retval None
  */
void TIM5_Configuration(void) {
  NVIC_InitTypeDef NVIC_InitStructure;
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  
  /* Prescaler value */
  uint16_t PrescalerValue = 0;
  
  /* Enable TIM5 Clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
  
  /* Enable the TIM5 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Compute the prescaler value */
  PrescalerValue = (uint16_t) (SystemCoreClock / 1000000) - 1;

  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 1000;
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);

  /* Prescaler configuration */
  TIM_PrescalerConfig(TIM5, PrescalerValue, TIM_PSCReloadMode_Immediate);

  /* TIM IT enable */
  TIM_ITConfig(TIM5, TIM_IT_Update, ENABLE);

  /* TIM6 enable counter */
  TIM_Cmd(TIM5, ENABLE); 
}

/**
  * @brief  Configures TIM6 clock and interrupt.
  * @param  None
  * @retval None
  */
void TIM6_Configuration(void) {
  NVIC_InitTypeDef NVIC_InitStructure;
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  
  /* Prescaler value */
  uint16_t PrescalerValue = 0;
  
  /* Enable TIM4 Clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
  
  /* Enable the TIM4 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM6_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Compute the prescaler value */
  PrescalerValue = (uint16_t) (SystemCoreClock / 1000000) - 1;

  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 1000;
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);

  /* Prescaler configuration */
  TIM_PrescalerConfig(TIM6, PrescalerValue, TIM_PSCReloadMode_Immediate);

  /* TIM IT enable */
  TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE);

  /* TIM6 enable counter */
  TIM_Cmd(TIM6, ENABLE); 
}


/**
  * @brief  Configures the USART1 periphral.
  * @param  None
  * @retval None
  */
void UART_Configuration(void) {
  GPIO_InitTypeDef GPIO_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  
  /* Enable GPIO, USART1 Clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
  
  /* Configure USART1 Tx as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* Configure USART1 Rx as input floating */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* Enable the USART1 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 4;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  /* USART1 configuration ------------------------------------------------------*/
  /* USART1 configured as follow:
        - BaudRate = 115200 baud  
        - Word Length = 8 Bits
        - One Stop Bit
        - No parity
        - Hardware flow control disabled (RTS and CTS signals)
        - Receive and transmit enabled
  */ 
  USART_InitTypeDef USART_InitStructure;
  
  USART_InitStructure.USART_BaudRate = 9600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  /* Configure USART1 */
  USART_Init(USART1, &USART_InitStructure);
  
  /* Enable USART1 Receive and Transmit interrupts */
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

  /* Enable the USART1 */
  USART_Cmd(USART1, ENABLE);   
}

/**
  * @brief  Configures the UART3/GPS periphral.
  * @param  None
  * @retval None
  */
void GPS_Configuration(void) {
  GPIO_InitTypeDef GPIO_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  
  /* Enable GPS GPIO Clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO, ENABLE);
  
  /* USART3 Remap Configuration */
  GPIO_PinRemapConfig(GPIO_PartialRemap_USART3, ENABLE);
  
    /* Enable GPS USART3 Clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
  
  /* Configure USART3 Tx as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  
  /* Configure USART3 Rx as input floating */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  
  /* Enable the USART3 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  /* USART3 configuration ------------------------------------------------------*/
  /* USART3 configured as follow:
        - BaudRate = 9600 baud  
        - Word Length = 8 Bits
        - One Stop Bit
        - No parity
        - Hardware flow control disabled (RTS and CTS signals)
        - Receive and transmit enabled
  */ 
  USART_InitTypeDef USART_InitStructure;
  
  USART_InitStructure.USART_BaudRate = 9600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  /* Configure USART3 */
  USART_Init(USART3, &USART_InitStructure);
  
  /* Enable USART3 Receive and Transmit interrupts */
  USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

  /* Enable the USART3 */
  USART_Cmd(USART3, ENABLE);   
}

void ATFC_GPIO_Configuration(void) {
  GPIO_InitTypeDef GPIO_InitStructure;
  
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE); // ATFC result from FPGA input
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE); // LED, BEEP output
  
  /* COnfigure PD10 as ATFC result incoming from FPGA */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  
  /* Configure PE15 as ATFC LED output */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOE, &GPIO_InitStructure);
  
  /* Configure PE01 as ATFC BEEP output */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOE, &GPIO_InitStructure);
}

/* RETARGET PRINTF */
int fputc(int ch, FILE *stream) {
  while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
  
  USART_SendData(USART1, (char) ch);

  return ch;
}

// Make a copy from raw collected data to temporary array
void CopyToTmpArray(uint8_t index) {
  int i;
  for(i=0; i<100; i++) {
    if(SyncFlag) {
      mAxisBuf.tmp_data_x_lcd[i] = mAxisData.data_x[i];
    } else {
      while(SyncFlag == 0);
      mAxisBuf.tmp_data_x_lcd[i] = mAxisData.data_x[i];
    }
    
    if(SyncFlag) {
      mAxisBuf.tmp_data_y_lcd[i] = mAxisData.data_y[i];
    } else {
      while(SyncFlag == 0);
      mAxisBuf.tmp_data_y_lcd[i] = mAxisData.data_y[i];
    }
    
    if(SyncFlag) {
      mAxisBuf.tmp_data_z_lcd[i] = mAxisData.data_z[i] - 4095;
    } else {
      while(SyncFlag == 0);
      mAxisBuf.tmp_data_z_lcd[i] = mAxisData.data_z[i] - 4095;
    }
    
    /* ATFC Algorithm result bit */
    if(SyncFlag) {
      mAxisBuf.ATFCBit_lcd[i] = mAxisData.ATFCBit[i];
    } else {
      while(SyncFlag == 0);
      mAxisBuf.ATFCBit_lcd[i] = mAxisData.ATFCBit[i];
    }
  }
}

// Copy untrimmed data to GAL array with its crude data trimmed
void CopyToTmpGalArray(uint8_t index) {
  // for code efficiency
  uint8_t arrIdx = index;
  
  if(mAxisBuf.tmp_data_x_lcd[arrIdx] & 0x2000)
    mGalBuf.tmp_gal_x[arrIdx] = (~mAxisBuf.tmp_data_x_lcd[arrIdx] + 1) & 0x1FFF;
  else
    mGalBuf.tmp_gal_x[arrIdx] = mAxisBuf.tmp_data_x_lcd[arrIdx] & 0x1FFF;
    
  if(mAxisBuf.tmp_data_y_lcd[arrIdx] & 0x2000)
    mGalBuf.tmp_gal_y[arrIdx] = (~mAxisBuf.tmp_data_y_lcd[arrIdx] + 1) & 0x1FFF;
  else
    mGalBuf.tmp_gal_y[arrIdx] = mAxisBuf.tmp_data_y_lcd[arrIdx] & 0x1FFF;
  
  if(mAxisBuf.tmp_data_z_lcd[arrIdx] & 0x2000)
    mGalBuf.tmp_gal_z[arrIdx] = (~mAxisBuf.tmp_data_z_lcd[arrIdx] + 1) & 0x1FFF;
  else
    mGalBuf.tmp_gal_z[arrIdx] = mAxisBuf.tmp_data_z_lcd[arrIdx] & 0x1FFF;
}

void CalculateGalAndCopyToGal(int index) {
  int TmpGalX = mGalBuf.tmp_gal_x[index] / 4;
  int TmpGalY = mGalBuf.tmp_gal_y[index] / 4;
  int TmpGalZ = mGalBuf.tmp_gal_z[index] / 4;
  
  // Calculate GAL
  tmp_gal = sqrt( ((TmpGalX) * (TmpGalX)) + ((TmpGalY) * (TmpGalY)) + ((TmpGalZ) * (TmpGalZ)) );
  
  // Copy to GAL Array
  mAxisData.data_g[index] = (int)tmp_gal;
}

void CheckSignAndToInt(int index) {
  int i;
  for(i=0; i<100; i++) {
    /* Axis X --------------------------------------*/
    //when it is positive value
    if(mAxisBuf.tmp_data_x_lcd[i] & 0x2000) { //when it is negative value
      mAxisBuf.tmp_data_x_lcd[i] = mAxisBuf.tmp_data_x_lcd[i] & 0xFFF;
      mAxisBuf.tmp_data_x_lcd[i] |= 0xFFFFF000;  // set MSB of int type
    }
    
    /* Axis Y --------------------------------------*/
    if(mAxisBuf.tmp_data_y_lcd[i] & 0x2000) {
      mAxisBuf.tmp_data_y_lcd[i] = mAxisBuf.tmp_data_y_lcd[i] & 0xFFF;
      mAxisBuf.tmp_data_y_lcd[i] |= 0xFFFFF000;  // set MSB of int type
    }
    
    /* Axis Z --------------------------------------*/
    if(mAxisBuf.tmp_data_z_lcd[i] & 0x2000) {
      mAxisBuf.tmp_data_z_lcd[i] = mAxisBuf.tmp_data_z_lcd[i] & 0xFFF;
      mAxisBuf.tmp_data_z_lcd[i] |= 0xFFFFF000;  // set MSB of int type
    } 
  }
}

// Make a copy from raw collected data to temporary array
void CopyToNetArray(int index) {
  int mIndex;
  mIndex = index;
  
  /* Axis X */
  if(SyncFlag) {
    mAxisNetBuf.axis_x_for_net[mIndex] = mAxisData.data_x[mIndex];
  } else {
    while(SyncFlag == 0);
    mAxisNetBuf.axis_x_for_net[mIndex] = mAxisData.data_x[mIndex];
  }
  
  /* Axis Y */
  if(SyncFlag) {
    mAxisNetBuf.axis_y_for_net[mIndex] = mAxisData.data_y[mIndex];
  } else {
    while(SyncFlag == 0);
    mAxisNetBuf.axis_y_for_net[mIndex] = mAxisData.data_y[mIndex];
  }
  
  /* Axis Z */
  if(SyncFlag) {
    mAxisNetBuf.axis_z_for_net[mIndex] = mAxisData.data_z[mIndex] - 4095;
  } else {
    while(SyncFlag == 0);
    mAxisNetBuf.axis_z_for_net[mIndex] = mAxisData.data_z[mIndex] - 4095;
  }
  
  /* ATFC Algorithm result bit */
  if(SyncFlag) {
    mAxisNetBuf.ATFCBit_net[mIndex] = mAxisData.ATFCBit[mIndex];
  } else {
    while(SyncFlag == 0);
    mAxisNetBuf.ATFCBit_net[mIndex] = mAxisData.ATFCBit[mIndex];
  }
}

void CheckSignAndToIntForNet(int index) {
  int mIndex;
  mIndex = index;
  
  /* Axis X --------------------------------------*/
  //when it is positive value
  if(mAxisNetBuf.axis_x_for_net[mIndex] & 0x2000) { //when it is negative value
    mAxisNetBuf.axis_x_for_net[mIndex] = mAxisNetBuf.axis_x_for_net[mIndex] & 0xFFF;
    mAxisNetBuf.axis_x_for_net[mIndex] |= 0xFFFFF000;  // set MSB of int type
  }
  
  /* Axis Y --------------------------------------*/
  if(mAxisNetBuf.axis_y_for_net[mIndex] & 0x2000) {
    mAxisNetBuf.axis_y_for_net[mIndex] = mAxisNetBuf.axis_y_for_net[mIndex] & 0xFFF;
    mAxisNetBuf.axis_y_for_net[mIndex] |= 0xFFFFF000;  // set MSB of int type
  }
  
  /* Axis Z --------------------------------------*/
  if(mAxisNetBuf.axis_z_for_net[mIndex] & 0x2000) {
    mAxisNetBuf.axis_z_for_net[mIndex] = mAxisNetBuf.axis_z_for_net[mIndex] & 0xFFF;
    mAxisNetBuf.axis_z_for_net[mIndex] |= 0xFFFFF000;  // set MSB of int type
  }
}

float ExtractMaxMeanACC(float data) {
  float mMeanACCn, result;
  mMeanACCn = data;
  
  if(mMeanACCn > MaxACC) MaxACC = mMeanACCn;
  
  if(MaxMeanCount < 50) {
    if(mMeanACCn > MaxMeanACC) MaxMeanACC = mMeanACCn; 
    MaxMeanCount++;
  } else {
    result = MaxMeanACC;
    MaxMeanACC = 0;
    MaxMeanCount = 0;
  }
  return result;
}

// Determine KMA scale
float DetermineKMA(int index) {
  int i, arrIdx, mX, mY, mZ;
  float result;
  
  arrIdx = index; 
  mX = mAxisBuf.tmp_data_x_lcd[arrIdx];
  mY = mAxisBuf.tmp_data_y_lcd[arrIdx];
  mZ = mAxisBuf.tmp_data_z_lcd[arrIdx];
  
  // get spot value of ACC(n)
  ACCn = (int)sqrt( (mX) * (mX) + (mY) * (mY) + (mZ) * (mZ) );
  
  if(KMACount < ACCnFifty - 1) {
    
    ACCnArr[KMACount] = ACCn;
    KMACount++;
    
  } else if(KMACount == ACCnFifty - 1) {
    
    ACCnArr[KMACount] = ACCn;
    for(i=0; i<KMACount; i++) {
      ACCnAddition += ACCnArr[i]; 
    }
    MeanACC = ACCnAddition / 50;
    result = ExtractMaxMeanACC(MeanACC);
    ACCnAddition = 0;   // Clear addition
    KMACount++;
    
  } else {  // KMACount >= 50
    
    ShiftArrayAndAddData(ACCnArr, ACCn, sizeof(ACCnArr) / sizeof(*ACCnArr));
    for(i=0; i<KMACount; i++) {
      ACCnAddition += ACCnArr[i]; 
    }
    MeanACC = ACCnAddition / 50;
    result = ExtractMaxMeanACC(MeanACC);
    ACCnAddition = 0;   // Clear addition
    
    if(KMACount >= 0x400) { // 1024
      KMACount = ACCnFifty;
    } else {
      KMACount++; 
    }
  }
  
  return result;
  
  /*
  int GalDataPlusOne = mAxisData.data_g[index+1];
  int GalDataIndex = mAxisData.data_g[index];
  
  if(GalDataPlusOne > GalDataIndex) {
    max_gal = mAxisData.data_g[index+1];
  } else {
    max_gal = max_gal;
  }
  */
}

/** @Function explanation
  * Copy the processed each axis data to selected data buffer
  * which should be big enough to contain a second of data and
  * is to be switched on every second to properly propagate
  * massive string data coming from the FPGA.
  */
void CopyToFatFsDataBuffer(int index) {  
  int arrIdx = index;

  mX = mAxisBuf.tmp_data_x_lcd[arrIdx];
  mY = mAxisBuf.tmp_data_y_lcd[arrIdx];
  mZ = mAxisBuf.tmp_data_z_lcd[arrIdx];
  
  int mHour, mMin, mSec, mTMSec;
  mHour = hour; mMin = minute; mSec = second; mTMSec = arrIdx;
  
  char strBuf[45];
  sprintf(strBuf, "%04d%02d%02d_%02d%02d%02d%02d_%+05d_%+05d_%+05d\r\n", 
          GetYearAndMergeToInt(), GetMonthAndMergeToInt(), GetDayAndMergeToInt(),
          mHour, mMin, mSec, mTMSec, mX, mY, mZ);
  
  // Count each second to determine FiveSecFlags.
  if(arrIdx == 99) {
    OneSecCount++;
    // When OneSecCount reach to 1 (first 1 second), set FiveSecFlag.
    // And when it reach to 2 (latter 1 second), reset FiveSecFlag and OneSecCount to recount from 0.
    
    int bytesWritten = 0;
    /* Save log to file process ------------------------------------------------*/
    /* Save process needs to be run every single cycle due to delay might occur */
    if(OneSecCount == 1) {
      printf("\r\nstrlen(DATA1_BUF) : %d\r\nstrlen(DATA2_BUF) : %d",
             strlen(DATA1_BUF), strlen(DATA2_BUF));
      
      // This routine means that DATA1_BUF is full and ready to flush out
      // be sure to empty out DATA1_BUF or will overflow and cause system to halt.
      
      /* Append first data for the duration of 1 second */
      bytesWritten = f_printf(&fsrc, DATA1_BUF);
      printf("\r\n%d of bytesWritten", bytesWritten);
      
      // Reset DATA1_BUF
      memset(DATA1_BUF, 0, sizeof(DATA1_BUF));
      
      printf("\r\nTimeElapsedInSecond = %d", TimeElapsedInSecond);
      
      /* Close the file */
      f_close(&fsrc);
      
      /* Create file in append mode in accordance with current minute */
      CreateFileAppendModeAccordingly(filePath, (RTC_GetCounter() % 3600) / 60);
      
      if(179 - TimeElapsedInSecond <= 0) {  // 179 (180sec) equals 3 minute
        TimeElapsedInSecond = 0;
        printf("\r\nFile Record Complete!");
      }
      EachSecFlag = true;
      
    } else if(OneSecCount == 2) {
      printf("\r\nstrlen(DATA1_BUF) : %d\r\nstrlen(DATA2_BUF) : %d", 
             strlen(DATA1_BUF), strlen(DATA2_BUF));
      
      /* Append another second of data */
      bytesWritten = f_printf(&fsrc, DATA2_BUF);
      printf("\r\n%d of bytesWritten", bytesWritten);
      
      // Reset DATA2_BUF
      memset(DATA2_BUF, 0, sizeof(DATA2_BUF));
      
      printf("\r\nTimeElapsedInSecond = %d", TimeElapsedInSecond);
      
      /* Close the file */
      f_close(&fsrc);
      
      /* Create file in append mode in accordance with current minute */
      CreateFileAppendModeAccordingly(filePath, (RTC_GetCounter() % 3600) / 60);
      
      if(179 - TimeElapsedInSecond <= 0) {
        TimeElapsedInSecond = 0;
        
        printf("\r\nFile Record Complete!");
      }
      EachSecFlag = false;
      OneSecCount = 0;
    }
    
    TimeElapsedInSecond++;
  }
  
  // following routine must be excuted every single cycle
  if(!EachSecFlag) {  // from 0 sec to right before 1 sec.
    strcat(DATA1_BUF, strBuf);
  } else {  // remaining 1 sec to the end of 2 sec.
    strcat(DATA2_BUF, strBuf);
  }
}

void ProcessParameterStream(void) {
  printf("\r\n\r\nModified Parameters are as follows\r\n%s", RX_BUF);
        
  char *Delimiter = "=\r\n";
  char *STRToken = NULL;
  
  STRToken = strtok((char*)RX_BUF, Delimiter); // IP[0]  ex) IP=192\r\n
  STRToken = strtok(NULL, Delimiter);
  IP[0] = atoi(STRToken);
  BKP_WriteBackupRegister(BKP_DR4, IP[0]); // Save IP[0]
  
  STRToken = strtok(NULL, Delimiter); // IP[1]
  STRToken = strtok(NULL, Delimiter);
  IP[1] = atoi(STRToken);
  BKP_WriteBackupRegister(BKP_DR5, IP[1]); // Save IP[1]
  
  STRToken = strtok(NULL, Delimiter); // IP[2]
  STRToken = strtok(NULL, Delimiter);
  IP[2] = atoi(STRToken);
  BKP_WriteBackupRegister(BKP_DR6, IP[2]); // Save IP[2]
  
  STRToken = strtok(NULL, Delimiter); // IP[3]
  STRToken = strtok(NULL, Delimiter);
  IP[3] = atoi(STRToken);
  BKP_WriteBackupRegister(BKP_DR7, IP[3]); // Save IP[3]
  
  STRToken = strtok(NULL, Delimiter); // Server PORT  < Default = 5050 >
  STRToken = strtok(NULL, Delimiter);
  EQDAS_Conf_PORT = atoi(STRToken);
  BKP_WriteBackupRegister(BKP_DR8, EQDAS_Conf_PORT); // Save Server PORT
  
  STRToken = strtok(NULL, Delimiter); // ATFC Algorithm Parameter Alpha < Default = 100 >
  STRToken = strtok(NULL, Delimiter);
  Alpha = atoi(STRToken);
  BKP_WriteBackupRegister(BKP_DR9, Alpha); // Save Parameter Alpha
  
  STRToken = strtok(NULL, Delimiter); // ATFC Algorithm Parameter Beta < Default = 1 >
  STRToken = strtok(NULL, Delimiter);
  Beta = atoi(STRToken);
  BKP_WriteBackupRegister(BKP_DR10, Beta); // Save Parameter Beta
  
  STRToken = strtok(NULL, Delimiter); // ATFC Algorithm Parameter Gamma < Default = 2 >
  STRToken = strtok(NULL, Delimiter);
  Gamma = atoi(STRToken);
  BKP_WriteBackupRegister(BKP_DR11, (uint16_t) Gamma); // Save Parameter Gamma
  
  STRToken = strtok(NULL, Delimiter); // ATFC Algorithm Parameter L < Default = 50 >
  STRToken = strtok(NULL, Delimiter);
  L = atoi(STRToken);
  BKP_WriteBackupRegister(BKP_DR12, L); // Save Parameter L
  
  STRToken = strtok(NULL, Delimiter); // ATFC Algorithm Parameter M < Default = 50 >
  STRToken = strtok(NULL, Delimiter);
  M = atoi(STRToken);
  BKP_WriteBackupRegister(BKP_DR13, M); // Save Parameter M
  
  STRToken = strtok(NULL, Delimiter); // ATFC Algorithm Parameter N < Default = 6 >
  STRToken = strtok(NULL, Delimiter);
  N = atoi(STRToken);
  BKP_WriteBackupRegister(BKP_DR14, N); // Save Parameter N
  
  STRToken = strtok(NULL, Delimiter); // ATFC Algorithm Parameter T < Default = 50 >
  STRToken = strtok(NULL, Delimiter);
  T = atoi(STRToken);
  BKP_WriteBackupRegister(BKP_DR15, T); // Save Parameter T
  
  STRToken = strtok(NULL, Delimiter); // ATFC Algorithm Parameter RefreshTime < Default = 100 >
  STRToken = strtok(NULL, Delimiter);
  RefreshTime = atoi(STRToken);
  BKP_WriteBackupRegister(BKP_DR16, RefreshTime); // Save Parameter RefreshTime
  
  char *PC_Buf = "Setting up the system is successfully done!\r\n";
  // Only when socket is established, allow send data
  if(getSn_SR(SOCK_TWO) == SOCK_ESTABLISHED) {
    /* send selected data */
    send(SOCK_TWO, (uint8_t*)PC_Buf, strlen(PC_Buf), (bool)false);
  }
}

void ProcessTextStream(uint8_t which, char *string, int index) {
  char *Delimiter = "_\r\n";
  char *TokenOne = NULL;
  char *TokenTwo = NULL;
  int arrIdx = index;
  
  switch(which) {
  case EQ_ONE :
    TokenOne = strtok(string, Delimiter); // result contains Date. ex) 20130428
    DAQBoardOne[arrIdx].Date = TokenOne;
    TokenOne = strtok(NULL, Delimiter); // result contains Time. ex) 21384729
    DAQBoardOne[arrIdx].Time = TokenOne;
    TokenOne = strtok(NULL, Delimiter); // result contains Axis X Data. ex) +4096
    DAQBoardOne[arrIdx].AxisX = TokenOne;
    TokenOne = strtok(NULL, Delimiter); // result contains Axis Y Data. ex) +4096
    DAQBoardOne[arrIdx].AxisY = TokenOne;
    TokenOne = strtok(NULL, Delimiter); // result contains Axis Z Data. ex) +4096
    DAQBoardOne[arrIdx].AxisZ = TokenOne;
    TokenOne = strtok(NULL, Delimiter); // result contains ATFC result Data.
    DAQBoardOne[arrIdx].ATFC = TokenOne;
    break;
    
  case EQ_TWO :
    TokenTwo = strtok(string, Delimiter); // result contains Date. ex) 20130428
    DAQBoardTwo[arrIdx].Date = TokenTwo;
    TokenTwo = strtok(NULL, Delimiter); // result contains Time. ex) 21384729
    DAQBoardTwo[arrIdx].Time = TokenTwo;
    TokenTwo = strtok(NULL, Delimiter); // result contains Axis X Data. ex) +4096
    DAQBoardTwo[arrIdx].AxisX = TokenTwo;
    TokenTwo = strtok(NULL, Delimiter); // result contains Axis Y Data. ex) +4096
    DAQBoardTwo[arrIdx].AxisY = TokenTwo;
    TokenTwo = strtok(NULL, Delimiter); // result contains Axis Z Data. ex) +4096
    DAQBoardTwo[arrIdx].AxisZ = TokenTwo;
    TokenTwo = strtok(NULL, Delimiter); // result contains ATFC result Data.
    DAQBoardTwo[arrIdx].ATFC = TokenTwo;
    break;
  case PC_Cli: 
    break;
  default: break;
  }
}

void SingleBoardDataToSendToPC(int which, int index) {
  char PC_Buf[45] = "";
  int mWhich, arrIdx;
  mWhich = which; arrIdx = index;
  
  switch(mWhich) {
  case EQ_ONE:
    sprintf(PC_Buf, "%s_%s_%s_%s_%s_%s\r\n",
            DAQBoardOne[arrIdx].Date,
            DAQBoardOne[arrIdx].Time,
            DAQBoardOne[arrIdx].AxisX,
            DAQBoardOne[arrIdx].AxisY,
            DAQBoardOne[arrIdx].AxisZ,
            DAQBoardOne[arrIdx].ATFC);
    break;
  case EQ_TWO:
    sprintf(PC_Buf, "%s_%s_%s_%s_%s_%s\r\n",
            DAQBoardTwo[arrIdx].Date,
            DAQBoardTwo[arrIdx].Time,
            DAQBoardTwo[arrIdx].AxisX,
            DAQBoardTwo[arrIdx].AxisY,
            DAQBoardTwo[arrIdx].AxisZ,
            DAQBoardTwo[arrIdx].ATFC);
    break;
  }
  
  // code for stacking algorithm which will combine data from two boards into one
  // Only when socket is established, allow send data
  if(getSn_SR(SOCK_TWO) == SOCK_ESTABLISHED) {  // SOCK_TWO = PC Client
    /* send selected data */
    send(SOCK_TWO, (uint8_t*)PC_Buf, strlen(PC_Buf), (bool)false);
  }
}

void MultipleBoardDataToSendToPC(int which, int orderOne, int orderTwo) {
  char PC_Buf[45] = "";
  int mWhich, arrIdxOne, arrIdxTwo;
  int OneX, OneY, OneZ, OneATFC, TwoX, TwoY, TwoZ, TwoATFC, AvgX, AvgY, AvgZ;
  mWhich = which; arrIdxOne = orderOne; arrIdxTwo = orderTwo;
  
  switch(mWhich) {
  case EQ_BOTH:
    OneX = atoi(DAQBoardOne[arrIdxOne].AxisX);
    OneY = atoi(DAQBoardOne[arrIdxOne].AxisY);
    OneZ = atoi(DAQBoardOne[arrIdxOne].AxisZ);
    OneATFC = atoi(DAQBoardOne[arrIdxOne].ATFC);
    
    TwoX = atoi(DAQBoardTwo[arrIdxTwo].AxisX);
    TwoY = atoi(DAQBoardTwo[arrIdxTwo].AxisY);
    TwoZ = atoi(DAQBoardTwo[arrIdxTwo].AxisZ);
    TwoATFC = atoi(DAQBoardTwo[arrIdxTwo].ATFC);

    /* Average 2 board's value wtih fancy name Site Stacking */    
    AvgX = (OneX + TwoX) >> 1;
    AvgY = (OneY + TwoY) >> 1;
    AvgZ = (OneZ + TwoZ) >> 1;
    
    /* Apply ATFC Algorithm to the value came out of site stacking average */
    if(GoATFCFlag) {
      // Apply ATFC Algorithm to Averaged Axis X data
      ATFCAlgorithm(AvgX);
      
      if(!ParamCompleteFlag) {  // wait until Threshold Reference work is done
        GPIO_WriteBit(GPIOE, GPIO_Pin_15, Bit_SET); // LED On signals it's not ready         
      } else {
        /* Routine on detection of ATFC Algorithm event */
        if(EventDetection && OneATFC && TwoATFC) {  // All 3 board detects ATFC only then it's ATFC final.
          GPIO_WriteBit(GPIOE, GPIO_Pin_1, Bit_SET); // BEEP on
          GPIO_WriteBit(GPIOE, GPIO_Pin_15, Bit_SET); // LED on
          mAlgorithmContainer = AvgZ;
        } else {
          GPIO_WriteBit(GPIOE, GPIO_Pin_1, Bit_RESET); // BEEP off
          GPIO_WriteBit(GPIOE, GPIO_Pin_15, Bit_RESET); // LED off
          mAlgorithmContainer = 0;
        }
      }
    } else {
      GPIO_WriteBit(GPIOE, GPIO_Pin_15, Bit_SET); // LED On signals it's not ready 
    }
    
    year = GetYearAndMergeToInt();
    month = GetMonthAndMergeToInt();
    day = GetDayAndMergeToInt();
    hour = THH; minute = TMM; second = TSS; tmsecond = 0;
    
    int mYear, mMonth, mDay, mHour, mMin, mSec, mTMSec;
    mYear = year; mMonth = month; mDay = day;
    mHour = hour; mMin = minute; mSec = second; mTMSec = tmsecond;
    sprintf(PC_Buf, "%04d%02d%02d_%02d%02d%02d%02d_%+05d_%+05d_%+05d_%+05d\r\n", 
                mYear, mMonth, mDay, mHour, mMin, mSec, mTMSec,
                AvgX, AvgY, AvgZ, mAlgorithmContainer);
    break;
  }
  
  // code for stacking algorithm which will combine data from two boards into one
  // Only when socket is established, allow send data
  if(getSn_SR(SOCK_TWO) == SOCK_ESTABLISHED) {  // SOCK_TWO = PC Client
    /* send selected data */
    send(SOCK_TWO, (uint8_t*)PC_Buf, strlen(PC_Buf), (bool)false);
  }
}

void ForceIPSetBySerial(void) {
  int ip0, ip1, ip2, ip3;
  ip0 = 0xFF; ip1 = 0xFF; ip2 = 0xFF; ip3 = 0xFF;
  
  printf("\r\n\r\n=================== IP Setting ====================");
  printf("\r\n  Please Set Year in 3 digits for each IP address : ");

  while (ip0 == 0xFF) {
    ip0 = USART_3DigitScanf(200);
  }
  printf("  IP[0] is set!");
  printf("\r\n  Please Set IP[1] : ");
  while (ip1 == 0xFF) {
    ip1 = USART_3DigitScanf(200);
  }
  printf("  IP[1] is set!");
  printf("\r\n  Please Set IP[2] : ");
  while (ip2 == 0xFF) {
    ip2 = USART_1DigitScanf(200);
  }
  printf("  IP[2] is set!");
  printf("\r\n  Please Set IP[3] : ");
  while (ip3 == 0xFF) {
    ip3 = USART_2DigitScanf(99);
  }
  printf("  IP[3] is set!");

  printf("\r\nEntered IP is %d.%d.%d.%d", ip0, ip1, ip2, ip3);
  IP[0] = ip0; IP[1] = ip1; IP[2] = ip2; IP[3] = ip3;
  
  /* Save year data to unresettable backup register addr. no. 2, 3 */
  BKP_WriteBackupRegister(BKP_DR4, IP[0]); // Save IP[0]
  BKP_WriteBackupRegister(BKP_DR5, IP[1]); // Save IP[1]
  BKP_WriteBackupRegister(BKP_DR6, IP[2]); // Save IP[2]
  BKP_WriteBackupRegister(BKP_DR7, IP[3]); // Save IP[3]
  
  printf("  Set IP complete!");
}

