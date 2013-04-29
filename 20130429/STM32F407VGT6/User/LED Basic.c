/**
  ******************************************************************************
  * @file    GPIO/IOToggle/main.c 
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

/** @addtogroup STM32F4xx_StdPeriph_Examples
  * @{
  */

/** @addtogroup IOToggle
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
GPIO_InitTypeDef  GPIO_InitStructure;

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static __IO uint32_t TimingDelay;
__IO uint16_t CCR1_Val = 84;
uint16_t PrescalerValue = 0;
uint16_t timercnt = 125;
uint16_t capture = 0;

unsigned short tchar[10] = {0x00, 0x00, 0x3a2, 0x237, 0x222, 0x00, 0xa2, 0xb7, 0x3a2, 0x00};
//unsigned short tchar1[10] = {0x00, 0x00, 0x00, 0x00, 0x1ca, 0x116, 0x16, 0x2b7, 0x176, 0xaa};
//unsigned short tchar2[10] = {0x00, 0x00, 0x00, 0x2b7, 0x2b2, 0x2b2, 0x2f5, 0x2b2, 0x2b7, 0x132};

//static unsigned char data[10] ={0x77,0xaf,0xdb,0xf8,0xfb,0xf8,0xdb,0xaf,0x77,0xff};      // >¤Ð<
//static unsigned char data1[10]={0xdf,0xdf,0xdb,0xf8,0xfb,0xf8,0xdb,0xaf,0x77,0xff};      // -¤Ð<
//static unsigned char data2[10]={0x77,0xaf,0xdb,0xfb,0xfb,0xfb,0xdb,0xaf,0x77,0xff};      // >_<
//static unsigned short data3[10]={0xcf,0x87,0x03,0x81,0xc0,0x81,0x03,0x87,0xcf,0xff};      // ¢¾
//static unsigned short data4[10]={0xcf,0xb7,0x7b,0xbd,0xde,0xbd,0x7b,0xb7,0xcf,0xff};    // ¢½
//static unsigned char data5[10]={0xff,0x00,0x00,0xfc,0xfc,0xfc,0xfc,0xfc,0xfc,0xff};         // L
//static unsigned char data6[10]={0xff,0xc3,0x99,0x3c,0x7e,0x7e,0x3c,0x99,0xc3,0xff};      // O
//static unsigned char data7[10]={0xff,0x07,0x03,0xf9,0xfc,0xfc,0xf9,0x03,0x07,0xff};       // V
//static unsigned char data8[10]={0xff,0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x66,0xff};     // E

/* Private function prototypes -----------------------------------------------*/
void GPIO_Config(void);
void NVIC_Config(void);
void TIM4_Config(void);
void out(unsigned short data[10]);
void Delay(__IO uint32_t nTime);
void TimingDelay_Decrement(void);

/* Private functions ---------------------------------------------------------*/

void GPIO_Config(void) {
  /* GPIOA Periph clock enable */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  
  /* GPIOE Periph clock enable */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
  
    /* TIM4 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

  /* Configure GPIOA in output pushpull mode */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* Configure PORTE in output pushpull mode */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOE, &GPIO_InitStructure);

  /* To achieve GPIO toggling maximum frequency, the following  sequence is mandatory. 
     You can monitor PG6 or PG8 on the scope to measure the output signal. 
     If you need to fine tune this frequency, you can add more GPIO set/reset 
     cycles to minimize more the infinite loop timing.
     This code needs to be compiled with high speed optimization option.  */
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
  
  /* Enable the sys_Tick gloabal Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = SysTick_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
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

void out(unsigned short data[10]) {
  for(int i=0; i<10 ;i++) {
    GPIO_Write(GPIOE, i);
    GPIO_Write(GPIOA, tchar[i]);
    Delay(1);
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

void SysTick_Handler(void) {
  TimingDelay_Decrement();
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
    
  GPIO_Config();
  NVIC_Config();
  TIM4_Config();
  
  while(1) {
    GPIO_Write(GPIOA, timercnt++);
    Delay(100);
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
