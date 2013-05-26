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

int Qt, Qf, ATFCn, THref, THn;
int SumQf, SumATFCn;
int THref, THn, THnpo, dTH;
int PreTRG, TRG;
bool FlagPreTRG;
int TrgOffLevel;
int PickingTime;
bool EventDetection, REC;

int Smp_Buf[50];
int ic = 1;

int SampleIndex = 0;
int L = 50; // sample
int T = 1000; // sample
int RefreshTime = 300;  // sample
int N = 6;  // sample
int M = 50; // sample

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
  int si;
  
ATFC:
  /* Calculation of ATFC */
  if(i < L-1) {
    if(i == 0) {
      Qt = abs(EQ_SAMPLE[i]);
      SumQf = abs(0 - EQ_SAMPLE[i]);
    } else {      
      Qt += abs(EQ_SAMPLE[i]);
      SumQf += abs(EQ_SAMPLE[i-1] - EQ_SAMPLE[i]);
    }
  } else {
    // run following routine first when i = 49
    if(i == L-1) {  // when i = 49
      Qt += abs(EQ_SAMPLE[i]);
      SumQf += abs(EQ_SAMPLE[i-1] - EQ_SAMPLE[i]);
      
      Qf = 100 * SumQf;
      ATFCn = Qt + Qf;
      /*
      printf("\r\n\n---------------------------");
      printf("\r\ni = %d", i);
      printf("\r\nQt = %d", Qt);
      printf("\r\nSumQf = %d", SumQf);
      printf("\r\nQf = %d", Qf);
      printf("\r\nATFCn = %d", ATFCn);
      printf("\r\n---------------------------");
      */
    } else {  // start from i = 50
      Qt = 0; // reset Qt
      SumQf = 0; // reset SumQf
      Qf = 0; // reset Qf
      ATFCn = 0; // reset ATFCn
      
      memset(Smp_Buf, 0, sizeof(Smp_Buf));  // clear sample buffer array
      for(si = 0; si < L; si++) {
        Smp_Buf[si] = EQ_SAMPLE[si+ic]; // initial ic = 1
        Qt += abs(Smp_Buf[si]);
        if(si == 0) {
          SumQf = abs(EQ_SAMPLE[si+ic-1] - EQ_SAMPLE[si+ic]);
        } else {
          SumQf += abs(Smp_Buf[si-1] - Smp_Buf[si]);
        }
      }
      Qf = 100 * SumQf;
      ATFCn = Qt + Qf;
      ic++;
      /*
      printf("\r\n---------------------------");
      printf("\r\ni = %d", i);
      printf("\r\nQt = %d", Qt);
      printf("\r\nSumQf = %d", SumQf);
      printf("\r\nQf = %d", Qf);
      printf("\r\nATFCn = %d", ATFCn);
      printf("\r\n---------------------------\r\n");
      */
    }
  }
  
  /* Parameter Initialization */
  if(i >= L-1 && i < T-1) {
    SumATFCn += ATFCn;
    /*
    if((i % 100) == 0) {
      printf("\r\n---------------------------");
      printf("\r\ni = %d", i);
      printf("\r\nATFCn = %d", ATFCn);
      printf("\r\nSumATFCn = %d", SumATFCn);
      printf("\r\n---------------------------");
    }
    */
  } else if(i >= T-1) {
    THref = SumATFCn >> 9; // instead of muliply of (2/T), divide 512 by shift 9 times
    THn = THref;
    
    printf("\r\n---------------------------");
    printf("\r\ni = %d", i);
    printf("\r\nATFCn = %d", ATFCn);
    printf("\r\nSumATFCn = %d", SumATFCn); 
    printf("\r\nTHref = %d", THref);
    printf("\r\n---------------------------");
    
    /* Instantaneous Threshold Value Contorl */
    dTH = ATFCn - THn;
    THnpo = THn + dTH;
    printf("\r\n---------------------------");
    printf("\r\ni = %d", i);
    printf("\r\nATFCn = %d", ATFCn);
    printf("\r\nTHn = %d", THn);
    printf("\r\ndTH = %d", dTH);
    printf("\r\nTHnpo = %d", THnpo);
    
    if(ATFCn >= THn) {
      PreTRG++;
      printf("\r\nPreTRG++");
    } else {
      PreTRG = 0;
      printf("\r\nPreTRG = 0");
    }
    
    if(ATFCn >= THref) {
      TRG++;
      printf("\r\nTRG++");
    } else {
      TRG = 0;
      printf("\r\nTRG = 0");
    }
    
    if( (PreTRG >= N) && (TRG > 0) ) {
      FlagPreTRG = true;
      printf("\r\nFlagPreTRG = true");
    } else {
      FlagPreTRG = false;
      printf("\r\nFlagPreTRG = false");
    }
    
    if( (FlagPreTRG == true) && (TRG >= M) ) {
      /* Compensation of Picking Time */
      TrgOffLevel = THref >> 1; // gamma * THref = 0.5 * THref = THref / 2 = THref >> 1
      
      /* Event Detection */
      EventDetection = true;
      
    RECORDING:
      /* Data Recording */
      REC = true;
      
      if(ATFCn <= TrgOffLevel) {
        REC = false;
        goto ATFC;
      } else {
        goto RECORDING;
      }
      
    } else {
      printf("\r\ngoto ATFC");
      goto ATFC;
    }
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
  
  /* TIM2 Configuration */
  TIM2_Configuration();
  
  /* UART1 Configuration */
  UART_Configuration();
  
  // When everything is set, print message
  printf("\r\n - System is ready - ");
  printf("\r\n - Begin ATFC Algorithm - ");
  
  while(1) {
    if(TimerCount >= 10) {
      TimerCount = 0;

      /* Excute ATFC Algorithm */
      ATFCAlgorithm(SampleIndex);
      SampleIndex++;
    }
  }
  
}

