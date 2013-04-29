/**
  ******************************************************************************
  * @file    STM32F407VGT6/User/Discovery Board Examples/main.c 
  * @author  Larry G
  * @version V1.0.0
  * @date    1-June-2012
  * @brief   ECG Project main program body
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
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include <stdio.h>

/** @addtogroup STM32F4_Discovery_Peripheral_Examples
  * @{
  */

/** @addtogroup ADC_ADC3_DMA
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define ADC3_DR_ADDRESS     ((uint32_t)0x4001224C)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* You can monitor the converted value by adding the variable "ADC3ConvertedValue" 
   to the debugger watch window */
__IO uint16_t ADC3ConvertedValue = 0;
__IO uint32_t ADC3ConvertedVoltage = 0;

// Timer & LED variable
__IO uint16_t CCR1_Val = 840;
uint8_t timercnt = 0;
uint16_t PrescalerValue = 0;
uint16_t capture = 0;
uint16_t ledData = 0x1000;

/* Private function prototypes -----------------------------------------------*/
void RCC_Config(void);
void GPIO_Config(void);
void NVIC_Config(void);
void TIM4_Config(void);
void UART4_Config(void);
void ADC3_CH12_DMA_Config(void);
void send2terminal(uint16_t data);
void putch(uint16_t character);
void putstr(unsigned char *str);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Configures the different system clocks.
  * @param  None
  * @retval None
  */
void RCC_Config(void) {
  /* GPIOA Periph clock enable */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
  
  /* GPIOD Periph clock enable */
  /* Green -> PD12, Orange -> PD13, Red -> PD14, Blue -> PD15 */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
  
  /* TIM4 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
  
  /* USART4 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
  
  /* Enable ADC3, DMA2 clocks */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3, ENABLE);
}

/**
  * @brief  Configures the general purpose input output port.
  * @param  None
  * @retval None
  */
void GPIO_Config(void) {
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Configure LD4, 3, 5, 6 Pins in output mode */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  
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
  
  /* Configure ADC3 Channel 12 pin as analog input */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
}

/**
  * @brief  Configures the nested vectored interrupt controller.
  * @param  None
  * @retval None
  */
void NVIC_Config(void) {
  NVIC_InitTypeDef NVIC_InitStructure;

  /* Enable the TIM4 gloabal Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  /* Enable the UART4 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);  
}

void TIM4_Config(void) {
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

void UART4_Config(void) {
  /* USART4 configuration ------------------------------------------------------*/
  /* USART4 configured as follow:
        - BaudRate = 9600 baud  
        - Word Length = 8 Bits
        - One Stop Bit
        - No parity
        - Hardware flow control disabled (RTS and CTS signals)
        - Receive and transmit enabled
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

/**
  * @brief  ADC3 channel12 with DMA configuration
  * @param  None
  * @retval None
  */
void ADC3_CH12_DMA_Config(void) {
  ADC_InitTypeDef       ADC_InitStructure;
  ADC_CommonInitTypeDef ADC_CommonInitStructure;
  DMA_InitTypeDef       DMA_InitStructure;
  
  /* ADC3 configuration *******************************************************/
  /*  - Enable peripheral clocks                                              */
  /*  - DMA2_Stream0 channel2 configuration                                   */
  /*  - Configure ADC Channel 12 pin as analog input                           */
  /*  - Configure ADC3 Channel 12                                              */

  /* DMA2 Stream0 channel0 configuration **************************************/
  DMA_InitStructure.DMA_Channel = DMA_Channel_2;  
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)ADC3_DR_ADDRESS;
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&ADC3ConvertedValue;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = 1;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(DMA2_Stream0, &DMA_InitStructure);
  DMA_Cmd(DMA2_Stream0, ENABLE);

  /* ADC Common Init **********************************************************/
  ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
  ADC_CommonInit(&ADC_CommonInitStructure);

  /* ADC3 Init ****************************************************************/
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfConversion = 1;
  ADC_Init(ADC3, &ADC_InitStructure);

  /* ADC3 regular channel12 configuration *************************************/
  ADC_RegularChannelConfig(ADC3, ADC_Channel_12, 1, ADC_SampleTime_3Cycles);

 /* Enable DMA request after last transfer (Single-ADC mode) */
  ADC_DMARequestAfterLastTransferCmd(ADC3, ENABLE);

  /* Enable ADC3 DMA */
  ADC_DMACmd(ADC3, ENABLE);

  /* Enable ADC3 */
  ADC_Cmd(ADC3, ENABLE);
  
  /* Start ADC3 Software Conversion */ 
  ADC_SoftwareStartConv(ADC3);
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
          if(ledData == 0x8000) ledData = 0x1000;
          else ledData <<= 1;
          break;
      case '2' :
          if(ledData == 0x1000) ledData = 0x8000;
          else ledData >>= 1;
          break;
      }
  }
}

void send2terminal(uint16_t data) {
  char buf[10], cnt;
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
    
  /* RCC configuration */
  RCC_Config();
  
  /* GPIO configuration */
  GPIO_Config();
              
  /* NVIC configuration */
  NVIC_Config();
  
  /* TIM4 configuration */
  TIM4_Config();
  
  /* UART4 configuration */
  UART4_Config();
  
  /* ADC3 DMA through Channel 12 Config */
  ADC3_CH12_DMA_Config();

  while (1) {
      if(timercnt >= 5) {
          timercnt = 0;
          
          /* convert the ADC value (from 0 to 0xFFF) to a voltage value (from 0V to 3.3V)*/
          //ADC3ConvertedVoltage = ADC3ConvertedValue * 3300 / 0xFFF;
          
          /* Send ADC3 Result data to UART4 */
          send2terminal(ADC3ConvertedValue/10);
      }
      
      // Set PORTD as a LED output for debug purpose
      //GPIO_Write(GPIOD, ledData);
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

  /* Infinite loop */
  while (1);
}
#endif

/**
  * @}
  */ 

/**
  * @}
  */ 

/******************************** END OF FILE ********************************/
