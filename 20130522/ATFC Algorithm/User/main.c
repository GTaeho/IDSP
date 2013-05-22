/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
typedef enum{false, true}bool;

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern short EQ_SAMPLE[];

extern volatile uint16_t TimerCount;

short Qt, Qf, ATFCn, THref, THn;

short SampleIndex = 0;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

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
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
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
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  /* USART1 configuration ------------------------------------------------------*/
  /* USART1 configured as follow:
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

  /* Configure USART1 */
  USART_Init(USART1, &USART_InitStructure);
  
  /* Enable USART1 Receive and Transmit interrupts */
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

  /* Enable the USART1 */
  USART_Cmd(USART1, ENABLE);   
}

/* RETARGET PRINTF */
int fputc(int ch, FILE *stream) {
  while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
  
  USART_SendData(USART1, (char) ch);

  return ch;
}

void ATFCAlgorithm(int addr) {
  int i = addr;  
  int L = 50;
  int T = 1000;
  int THref, THn, THn1;
  int RefreshTime = addr;
  int SumQf = 0, SumATFCn = 0;
  int PreTRG, TRG;
  bool FlagPreTRG;
  
  printf("\r\n\nSampleIndex : %d", SampleIndex);
  /* Calculation of ATFC */
  if(i < L) {
    printf("\r\ni = %d", i);
    Qt += abs(EQ_SAMPLE[i]);
    if( i == 0 && (0 - EQ_SAMPLE[i]) && 0x8000) {  // when it comes negative and first time
      // do 2's complement to represent negative value correctly
      SumQf += abs( ~(0 - EQ_SAMPLE[i]) + 1 );
      printf("\r\nSumQf = %d", SumQf);
    } else if(i == 0) {
      SumQf += abs(0 - EQ_SAMPLE[i]);
    }
    if(i > 0) SumQf += abs(EQ_SAMPLE[i-1] - EQ_SAMPLE[i]);
    i++;
  }
  Qf = 100 * SumQf;
  ATFCn = Qt + Qf;
  
  printf("\r\nATFCn = %d", ATFCn);  // DEBUG
  
  /* Parameter Initialization */
  for(i=0; i<T; i++) {
    SumATFCn += ATFCn;
  }
  THref = (2 / T) * SumATFCn;
  THn = THref;
  
  printf("THref = %d", THref);  // DEBUG
  
  if(RefreshTime >= 300) {
    /* Reference Threshold Value Control */
    SumATFCn = 0;
    for(i=0; i<T; i++) {
      SumATFCn += ATFCn;
    }
    THref = (2 / T) * SumATFCn;
  } else {
    /* Instantaneous Threshold Value Control */
    THn1 = THn + (ATFCn - THn);
  }
  
  if(ATFCn >= THn) PreTRG += 1;
  else PreTRG = 0;
  
  if(ATFCn >= THref) TRG += 1;
  else TRG = 0;
  
  if(PreTRG >= 6 && TRG > 0) FlagPreTRG = true;
  else FlagPreTRG = false;
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
  
  /* TIM2 Configuration */
  TIM2_Configuration();
  
  /* UART1 Configuration */
  UART_Configuration();
  
  // When everything is set, print message
  printf("\r\n - System is ready - ");
  printf("\r\n - Begin ATFC Algorithm - ");
  
  while(1) {
    if(TimerCount >= 1000) {
      TimerCount = 0;

      /* Excute ATFC Algorithm */
      ATFCAlgorithm(SampleIndex);
      SampleIndex++;
    }
  }
  
}

