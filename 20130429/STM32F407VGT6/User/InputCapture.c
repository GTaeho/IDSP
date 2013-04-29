/**
  ******************************************************************************
  * @file    TIM/InputCapture/main.c 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    30-September-2011
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "stdio.h"

/** @addtogroup STM32F4xx_StdPeriph_Examples
  * @{
  */

/** @addtogroup TIM_Input_Capture
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint16_t IC3ReadValue1 = 0, IC3ReadValue2 = 0;
__IO uint16_t CaptureNumber = 0;
__IO uint32_t Capture = 0;
__IO uint32_t TIM1Freq = 0;
__IO uint32_t TimingDelay;

// Timer & LED variable
__IO uint16_t CCR1_Val = 84;
uint8_t timercnt = 0;
uint16_t PrescalerValue = 0;
uint16_t capture = 0;

/* Private function prototypes -----------------------------------------------*/
void RCC_Configuration(void);
void NVIC_Configuration(void);
void GPIO_Configuration(void);
void TIM1_Configuration(void);
void TIM4_Configuration(void);
void UART4_Configuration(void);
void send2terminal(uint16_t data);
void putch(uint16_t character);
void putstr(unsigned char *str);
void Delay(__IO uint32_t nTime);
void TimingDelay_Decrement(void);

/* Private functions ---------------------------------------------------------*/

void RCC_Configuration(void) {
 /* TIM1 clock enable for Capture */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
  
  /* TIM4 clock enable for UART4 */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
  
  /* GPIOC Periph clock enable */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

  /* GPIOE clock enable */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);   
  
  /* USART4 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
}

void NVIC_Configuration(void) {
  NVIC_InitTypeDef NVIC_InitStructure;
  
  /* Enable the TIM1 global Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM1_CC_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);    
  
  /* Enable the TIM4 gloabal Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  /* Enable the UART4 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

void GPIO_Configuration(void) {
  GPIO_InitTypeDef GPIO_InitStructure;
    
  /* TIM1 channel 2 pin (PE.11) configuration */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOE, &GPIO_InitStructure);

  /* Connect TIM pins to AF2 */
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource11, GPIO_AF_TIM1);
  
  /* configure AF */
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_UART4);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_UART4);
    
  /* Configure UART4 Tx as alternate function */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  /* Configure UART4 Rx as input mode */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
}

/**
  * @brief  Configure the TIM1 Pins.
  * @param  None
  * @retval None
  */
void TIM1_Configuration(void) {
  /* TIM1 configuration: Input Capture mode ---------------------
     The external signal is connected to TIM1 CH2 pin (PE.11)  
     The Rising edge is used as active edge,
     The TIM1 CCR2 is used to compute the frequency value 
  ------------------------------------------------------------ */
  TIM_ICInitTypeDef  TIM_ICInitStructure;

  TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV8;
  TIM_ICInitStructure.TIM_ICFilter = 0x0;

  TIM_ICInit(TIM1, &TIM_ICInitStructure);
  
  /* TIM enable counter */
  TIM_Cmd(TIM1, ENABLE);

  /* Enable the CC2 Interrupt Request */
  TIM_ITConfig(TIM1, TIM_IT_CC2, ENABLE);
}

void TIM4_Configuration(void) {
 /* -----------------------------------------------------------------------
    TIM4 Configuration: Output Compare Timing Mode:
    
    In this example TIM4 input clock (TIM4CLK) is set to 2 * APB1 clock (PCLK1), 
    since APB1 prescaler is different from 1.   
      TIM4CLK = 2 * PCLK1  
      PCLK1 = HCLK / 4 
      => TIM4CLK = HCLK / 2 = SystemCoreClock / 2
	  (HCLK = 168MHz, PCLK1 = 42MHz, TIM4LCK = 84MHz)
          
    To get TIM4 counter clock at 84 KHz, the prescaler is computed as follows:
       Prescaler = ((TIM4CLK / 2) / TIM4 counter clock) - 1
       Prescaler = ((SystemCoreClock /2) /84 KHz) - 1

    No need to focus on what's the value of prescaler, but more on TIM4 Counter clock
    In this case, it is 84KHz and this is divided by CCR1_Val then is led to final
    Resolution you are looking into.
    
    CC1 update rate = TIM4 counter clock / CCR1_Val = 5 Hz
    ==> Toggling frequency = 2.5 Hz

    Note:
     SystemCoreClock variable holds HCLK frequency and is defined in system_stm32f4xx.c file.
     Each time the core clock (HCLK) changes, user had to call SystemCoreClockUpdate()
     function to update SystemCoreClock variable value. Otherwise, any configuration
     based on this variable will be incorrect.
  ----------------------------------------------------------------------- */
    
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  TIM_OCInitTypeDef  TIM_OCInitStructure;

  /* Compute the prescaler value */
  PrescalerValue = (uint16_t) ((SystemCoreClock / 2) / 84000) - 1;

  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 65535;
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

  /* Prescaler configuration */
  TIM_PrescalerConfig(TIM4, PrescalerValue, TIM_PSCReloadMode_Immediate);

  /* Output Compare Timing Mode configuration: Channel1 */
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = CCR1_Val;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

  TIM_OC1Init(TIM4, &TIM_OCInitStructure);

  TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Disable);
  
  /* TIM Interrupts enable */
  TIM_ITConfig(TIM4, TIM_IT_CC1, ENABLE);

  /* TIM4 enable counter */
  TIM_Cmd(TIM4, ENABLE);
}

void UART4_Configuration(void) {
  /* USART4 configuration ------------------------------------------------------*/
  /* USART4 configured as follow:
        - BaudRate = 115200 baud  
        - Word Length = 8 Bits
        - One Stop Bit
        - No parity
        - Hardware flow control disabled (RTS and CTS signals)
        - Receive enabled
  */ 
  USART_InitTypeDef USART_InitStructure;
    
  USART_InitStructure.USART_BaudRate = 115200;
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
}

void SysTick_Handler(void) {
  TimingDelay_Decrement();
}

void send2terminal(uint16_t data) {
  char buf[9], cnt;
  sprintf(buf, "%d", (uint16_t)data);
	
  for(cnt=0;buf[cnt];cnt++) {
    putch(buf[cnt]);
  }
  
  putstr("\n");
}

void putch(uint16_t character) {
  while (USART_GetFlagStatus(UART4, USART_FLAG_TXE) == RESET);
  
  USART_SendData(UART4, character);
}

void putstr(unsigned char *str) {
  while(*str != '\0')  {
    putch(*str);
    str++;
  }
}

/**
  * @brief  Inserts a delay time.
  * @param  nTime: specifies the delay time length, in milliseconds.
  * @retval None
  */
void Delay(__IO uint32_t nTime) { 
  TimingDelay = nTime;

  while(TimingDelay != 0);
}

/**
  * @brief  Decrements the TimingDelay variable.
  * @param  None
  * @retval None
  */
void TimingDelay_Decrement(void) {
  if (TimingDelay != 0x00) {
    TimingDelay--;
  }
}

/**
  * @brief  This function handles TIM1 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM1_CC_IRQHandler(void) { 
  if(TIM_GetITStatus(TIM1, TIM_IT_CC2) == SET) {
    /* Clear TIM1 Capture compare interrupt pending bit */
    TIM_ClearITPendingBit(TIM1, TIM_IT_CC2);
    if(CaptureNumber == 0) {
      /* Get the Input Capture value */
      IC3ReadValue1 = TIM_GetCapture2(TIM1);
      CaptureNumber = 1;
    } else if(CaptureNumber == 1) {
      /* Get the Input Capture value */
      IC3ReadValue2 = TIM_GetCapture2(TIM1);
      
      /* Capture computation */
      if (IC3ReadValue2 > IC3ReadValue1) {
        Capture = (IC3ReadValue2 - IC3ReadValue1);
      } else if (IC3ReadValue2 < IC3ReadValue1) {
        Capture = ((0xFFFF - IC3ReadValue1) + IC3ReadValue2);
      } else {
        Capture = 0;
      }
      /* Frequency computation */
      TIM1Freq = (uint32_t) SystemCoreClock / Capture;
      CaptureNumber = 0;
    }
  }
}

/**
  * @brief  This function handles TIM4 global interrupt request.
    * @param  None
  * @retval None
  */
void TIM4_IRQHandler(void) {
  if (TIM_GetITStatus(TIM4, TIM_IT_CC1) != RESET) {
    TIM_ClearITPendingBit(TIM4, TIM_IT_CC1);
    capture = TIM_GetCapture1(TIM4);
    TIM_SetCompare1(TIM4, capture + CCR1_Val);
    
    timercnt++;
  }
}

/**
  * @brief  This function handles UART4 global interrupt request.
  * @param  None
  * @retval None
  */
void UART4_IRQHandler(void) {
  if(USART_GetITStatus(UART4, USART_IT_RXNE) != RESET) {
    switch(USART_ReceiveData(UART4)) {
      case '1' :
          break;
      case '2' :
          break;
      }
  }
}

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void) {
  /*!< At this stage the microcontroller clock setting is already configured, 
       this is done through SystemInit() function which is called from startup
       file (startup_stm32f4xx.s) before to branch to application main.
       To reconfigure the default setting of SystemInit() function, refer to
       system_stm32f4xx.c file
     */
    
  /* Setup SysTick Timer for 1 msec interrupts.
     ------------------------------------------
    1. The SysTick_Config() function is a CMSIS function which configure:
       - The SysTick Reload register with value passed as function parameter.
       - Configure the SysTick IRQ priority to the lowest value (0x0F).
       - Reset the SysTick Counter register.
       - Configure the SysTick Counter clock source to be Core Clock Source (HCLK).
       - Enable the SysTick Interrupt.
       - Start the SysTick Counter.
    
    2. You can change the SysTick Clock source to be HCLK_Div8 by calling the
       SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8) just after the
       SysTick_Config() function call. The SysTick_CLKSourceConfig() is defined
       inside the misc.c file.

    3. You can change the SysTick IRQ priority by calling the
       NVIC_SetPriority(SysTick_IRQn,...) just after the SysTick_Config() function 
       call. The NVIC_SetPriority() is defined inside the core_cm4.h file.

    4. To adjust the SysTick time base, use the following formula:
                            
         Reload Value = SysTick Counter Clock (Hz) x  Desired Time base (s)
    
       - Reload Value is the parameter to be passed for SysTick_Config() function
       - Reload Value should not exceed 0xFFFFFF
   */
  if (SysTick_Config(SystemCoreClock / 1000)) {
    /* Capture error */ 
    while (1);
  }
  
  /* RCC Configuration */  
  RCC_Configuration();
    
  /* NVIC Configuration */
  NVIC_Configuration();
  
  /* GPIO Configuration */
  GPIO_Configuration();
       
  /* TIM1 Configuration */
  TIM1_Configuration();
  
  /* TIM4 Configuration */
  TIM4_Configuration();
  
  /* UART4 Configuration */
  UART4_Configuration();
  
  Delay(1000);
  putstr("AT+BTSCAN,3,0\r");
  Delay(500);

  while (1) {
    if(timercnt >= 250) {
      timercnt = 0;
      
      send2terminal(Capture);
    }
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
void assert_failed(uint8_t* file, uint32_t line) {
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  while (1);
}

#endif

/**
  * @}
  */ 

/**
  * @}
  */ 

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
