/**
  ******************************************************************************
  * @file    TIM_TimeBase/main.c 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    19-September-2011
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
#include "stm32f4_discovery.h"

/** @addtogroup STM32F4_Discovery_Peripheral_Examples
  * @{
  */

/** @addtogroup TIM_TimeBase
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
__IO uint16_t CCR1_Val = 42E3;
__IO uint16_t CCR2_Val = 8400;
__IO uint16_t CCR3_Val = 2000;
__IO uint16_t CCR4_Val = 100;

uint16_t PrescalerValue = 0;
uint16_t capture = 0;

volatile short OCC1SEC;

volatile uint32_t SYSCLK_Freq;
volatile uint32_t HCLK_Freq;
volatile uint32_t PCLK1_Freq;
volatile uint32_t PCLK2_Freq;

/* Private function prototypes -----------------------------------------------*/
void TIM_Config(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Configure the RCC.
  * @param  None
  * @retval None
  */
void RCC_Config(void) {
  RCC_ClocksTypeDef RCC_ClockFreq;
  
  RCC_GetClocksFreq(&RCC_ClockFreq);
  
  SYSCLK_Freq = RCC_ClockFreq.SYSCLK_Frequency;
  HCLK_Freq = RCC_ClockFreq.HCLK_Frequency;
  PCLK1_Freq = RCC_ClockFreq.PCLK1_Frequency;
  PCLK2_Freq = RCC_ClockFreq.PCLK2_Frequency;
  
  /* Enable the GPIO_LED Clock */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

  /* TIM3 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
  
  /* Enable GPIOA clock */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  
  /* Enable SYSCFG clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
}

/**
  * @brief  Configure the GPIO.
  * @param  None
  * @retval None
  */
void GPIO_Config(void) {
  GPIO_InitTypeDef   GPIO_InitStructure;
  
  /* Configure the GPIO_LED pin */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  
  /* Initialize Leds mounted on STM32F4-Discovery board */
  //STM_EVAL_LEDInit(LED3);
  //STM_EVAL_LEDInit(LED4);
  //STM_EVAL_LEDInit(LED5);
  //STM_EVAL_LEDInit(LED6);

  /* Turn on LED3, LED4, LED5 and LED6 */
  //STM_EVAL_LEDOn(LED3);
  //STM_EVAL_LEDOn(LED4);
  //STM_EVAL_LEDOn(LED5);
  //STM_EVAL_LEDOn(LED6);
  
  /* Configure PA0 pin as input floating */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* Connect EXTI Line0 to PA0 pin */
  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0);
}

/**
  * @brief  Configure the EXTI.
  * @param  None
  * @retval None
  */
void EXTI_Config(void) {
  EXTI_InitTypeDef   EXTI_InitStructure;
  
  /* Configure EXTI Line0 */
  EXTI_InitStructure.EXTI_Line = EXTI_Line0;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);
}

/**
  * @brief  Configure the NVIC.
  * @param  None
  * @retval None
  */
void NVIC_Config(void) {
  NVIC_InitTypeDef NVIC_InitStructure;
  
  /* Enable the TIM3 gloabal Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  /* Enable and set EXTI Line0 Interrupt to the lowest priority */
  NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

/**
  * @brief  Configure the TIM3.
  * @param  None
  * @retval None
  */
void TIM3_Config(void) {
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  TIM_OCInitTypeDef  TIM_OCInitStructure;

  /* -----------------------------------------------------------------------
    TIM3 Configuration: Output Compare Timing Mode:
    
    In this example TIM3 input clock (TIM3CLK) is set to 2 * APB1 clock (PCLK1), 
    since APB1 prescaler is different from 1.   
      TIM3CLK = 2 * PCLK1  
      PCLK1 = HCLK / 4 
      => TIM3CLK = HCLK / 2 = SystemCoreClock /2
	  (HCLK = 168MHz, PCLK1 = 42MHz, TIM3LCK = 84MHz)
          
    To get TIM3 counter clock at 84 KHz, the prescaler is computed as follows:
       Prescaler = (TIM3CLK / TIM3 counter clock) - 1
       Prescaler = ((SystemCoreClock /2) /84 MHz) - 1
                                              
    CC1 update rate = TIM3 counter clock / CCR1_Val = 5 Hz
    ==> Toggling frequency = 2.5 Hz
    
    C2 update rate = TIM3 counter clock / CCR2_Val = 10 Hz
    ==> Toggling frequency = 5 Hz
    
    CC3 update rate = TIM3 counter clock / CCR3_Val = 42 Hz
    ==> Toggling frequency = 21 Hz
    
    CC4 update rate = TIM3 counter clock / CCR4_Val = 84 Hz
    ==> Toggling frequency = 42 Hz

    Note: 
     SystemCoreClock variable holds HCLK frequency and is defined in system_stm32f4xx.c file.
     Each time the core clock (HCLK) changes, user had to call SystemCoreClockUpdate()
     function to update SystemCoreClock variable value. Otherwise, any configuration
     based on this variable will be incorrect.
  ----------------------------------------------------------------------- */  

  /* Compute the prescaler value */
  PrescalerValue = (uint16_t) ((SystemCoreClock / 2) / 42E6) - 1;

  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 65535;
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

  /* Prescaler configuration */
  TIM_PrescalerConfig(TIM3, PrescalerValue, TIM_PSCReloadMode_Immediate);

  /* Output Compare Timing Mode configuration: Channel1 */
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = CCR1_Val;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

  TIM_OC1Init(TIM3, &TIM_OCInitStructure);

  TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Disable);

  /* Output Compare Timing Mode configuration: Channel2 */
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = CCR2_Val;

  TIM_OC2Init(TIM3, &TIM_OCInitStructure);

  TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Disable);

  /* Output Compare Timing Mode configuration: Channel3 */
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = CCR3_Val;

  TIM_OC3Init(TIM3, &TIM_OCInitStructure);

  TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Disable);

  /* Output Compare Timing Mode configuration: Channel4 */
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = CCR4_Val;

  TIM_OC4Init(TIM3, &TIM_OCInitStructure);

  TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Disable);
   
  /* TIM Interrupts enable */
  TIM_ITConfig(TIM3, TIM_IT_CC1 | TIM_IT_CC2 | TIM_IT_CC3 | TIM_IT_CC4, ENABLE);

  /* TIM3 enable counter */
  TIM_Cmd(TIM3, ENABLE);
}

/**
  * @brief  This function handles TIM3 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM3_IRQHandler(void) {
  if (TIM_GetITStatus(TIM3, TIM_IT_CC1) != RESET) {
    TIM_ClearITPendingBit(TIM3, TIM_IT_CC1);

    /* LED4 toggling with frequency = 4.57 Hz */
    //STM_EVAL_LEDToggle(LED4);
    //capture = TIM_GetCapture1(TIM3);
    //TIM_SetCompare1(TIM3, capture + CCR1_Val);
    OCC1SEC++;
    
  } else if (TIM_GetITStatus(TIM3, TIM_IT_CC2) != RESET) {
    TIM_ClearITPendingBit(TIM3, TIM_IT_CC2);

    /* LED3 toggling with frequency = 9.15 Hz */
    //STM_EVAL_LEDToggle(LED3);
    //capture = TIM_GetCapture2(TIM3);
    //TIM_SetCompare2(TIM3, capture + CCR2_Val);
  } else if (TIM_GetITStatus(TIM3, TIM_IT_CC3) != RESET) {
    TIM_ClearITPendingBit(TIM3, TIM_IT_CC3);

    /* LED5 toggling with frequency = 18.31 Hz */
    //STM_EVAL_LEDToggle(LED5);
    //capture = TIM_GetCapture3(TIM3);
    //TIM_SetCompare3(TIM3, capture + CCR3_Val);
  } else {
    TIM_ClearITPendingBit(TIM3, TIM_IT_CC4);

    /* LED6 toggling with frequency = 36.62 Hz */
    //STM_EVAL_LEDToggle(LED6);
    //capture = TIM_GetCapture4(TIM3);
    //TIM_SetCompare4(TIM3, capture + CCR4_Val);
  }
}

/**
  * @brief  This function handles External line 0 interrupt request.
  * @param  None
  * @retval None
  */
void EXTI0_IRQHandler(void) {
  if(EXTI_GetITStatus(EXTI_Line0) != RESET) {
    /* Toggle LED4 */
    STM_EVAL_LEDToggle(LED6);
    
    /* Clear the EXTI line 0 pending bit */
    EXTI_ClearITPendingBit(EXTI_Line0);
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
  
  RCC_Config();
  GPIO_Config();
  EXTI_Config();
  NVIC_Config();
  TIM3_Config();

  while (1) {
    if(OCC1SEC >= 100) {
      OCC1SEC = 0;
      STM_EVAL_LEDToggle(LED3);
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
