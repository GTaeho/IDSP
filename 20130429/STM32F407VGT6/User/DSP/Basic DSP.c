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
#include "arm_math.h" 
#include "math_helper.h"

/** @addtogroup STM32F4xx_StdPeripheral
  * @{
  */

/** @addtogroup USART_HyperTerminal_Interrupt
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define TEST_LENGTH_SAMPLES             320
#define SNR_THRESHOLD_F32	        150.0f
#define BLOCK_SIZE			32 
#define NUM_TAPS			29

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
__IO uint16_t CCR1_Val = 16800;
uint16_t ledData = 0x1000;
uint8_t timercnt = 0;
uint16_t PrescalerValue = 0;
uint16_t capture = 0;

/* Digital LPF Variables */
uint32_t i;
arm_fir_instance_f32 S;
arm_status status;
float32_t  *inputF32, *outputF32;

/* Private function prototypes -----------------------------------------------*/
void RCC_Config(void);
void GPIO_Config(void);
void NVIC_Config(void);
void TIM4_Config(void);
void UART4_Config(void);
void DLPF_Config(void);
void putch(uint16_t character);
void putstr(unsigned char *str);

/* ------------------------------------------------------------------- 
 * The input signal and reference output (computed with MATLAB)
 * are defined externally in arm_fir_lpf_data.c
 * ------------------------------------------------------------------- */ 
extern float32_t testInput_f32_1kHz_15kHz[TEST_LENGTH_SAMPLES]; 
extern float32_t refOutput[TEST_LENGTH_SAMPLES];

/* ------------------------------------------------------------------- 
 * Declare Test output buffer
 * ------------------------------------------------------------------- */ 
static float32_t testOutput[TEST_LENGTH_SAMPLES]; 
 
/* ------------------------------------------------------------------- 
 * Declare State buffer of size (numTaps + blockSize - 1) 
 * ------------------------------------------------------------------- */ 
static float32_t firStateF32[BLOCK_SIZE + NUM_TAPS - 1];
 
/* ---------------------------------------------------------------------- 
** FIR Coefficients buffer generated using fir1() MATLAB function. 
** fir1(28, 6/24)
** ------------------------------------------------------------------- */ 
const float32_t firCoeffs32[NUM_TAPS] = { 
-0.0018225230f, -0.0015879294f, +0.0000000000f, +0.0036977508f, +0.0080754303f, +0.0085302217f, -0.0000000000f, -0.0173976984f, 
-0.0341458607f, -0.0333591565f, +0.0000000000f, +0.0676308395f, +0.1522061835f, +0.2229246956f, +0.2504960933f, +0.2229246956f, 
+0.1522061835f, +0.0676308395f, +0.0000000000f, -0.0333591565f, -0.0341458607f, -0.0173976984f, -0.0000000000f, +0.0085302217f, 
+0.0080754303f, +0.0036977508f, +0.0000000000f, -0.0015879294f, -0.0018225230f 
}; 

/* ------------------------------------------------------------------ 
 * Global variables for FIR LPF
 * ------------------------------------------------------------------- */ 
uint32_t blockSize = BLOCK_SIZE; 
uint32_t numBlocks = TEST_LENGTH_SAMPLES/BLOCK_SIZE;

float32_t  snr;

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
}

void DLPF_Config(void) {
  /* Initialize input and output buffer pointers */
  inputF32 = &testInput_f32_1kHz_15kHz[0];
  outputF32 = &testOutput[0];

  /* Call FIR init function to initialize the instance structure. */
  arm_fir_init_f32(&S, NUM_TAPS, (float32_t *)&firCoeffs32[0], &firStateF32[0], blockSize);
 
  /* ---------------------------------------------------------------------- 
  ** Call the FIR process function for every blockSize samples  
  ** ------------------------------------------------------------------- */ 

  for(i=0; i < numBlocks; i++) {
    arm_fir_f32(&S, inputF32 + (i * blockSize), outputF32 + (i * blockSize), blockSize);  
  } 
 
  /* ---------------------------------------------------------------------- 
  ** Compare the generated output against the reference output computed
  ** in MATLAB.
  ** ------------------------------------------------------------------- */ 

  /* ---------------------------------------------------------------------- 
  ** arm_snr_f32 method from math_helper.c
  ** SNR stands for Signal to Noise Ratio which means
  ** The ratio between Signal against proportionate to Noise level.
  ** ------------------------------------------------------------------- */ 
  snr = arm_snr_f32(&refOutput[0], &testOutput[0], TEST_LENGTH_SAMPLES); 
 
  if (snr < SNR_THRESHOLD_F32) {
      status = ARM_MATH_TEST_FAILURE;
      GPIO_WriteBit(GPIOD, GPIO_Pin_12, Bit_SET);
  } else {
      status = ARM_MATH_SUCCESS;
      GPIO_WriteBit(GPIOD, GPIO_Pin_14, Bit_SET);
  }

  /* ---------------------------------------------------------------------- 
  ** Loop here if the signal does not match the reference output.
  ** ------------------------------------------------------------------- */ 
	 
  if( status != ARM_MATH_SUCCESS) {
      while(1);
  }
}

void putch(uint16_t character) {
  while (USART_GetFlagStatus(UART4, USART_FLAG_TXE) == RESET);
  
  USART_SendData(UART4, character);
}

void putstr(unsigned char *str) {
  while (*str != '\0')  {
    putch(*str);
    str++;
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

/**
  * @brief   Main program
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
  
  /* Digital Low Pass Filter Configuration */
  DLPF_Config();
  
  while (1) {
      if(timercnt >= 5) {
          timercnt = 0;
          
          if(ledData >= 0x8000) ledData = 0x1000;
          else ledData <<= 1;
          
          putstr("Larry");
      }
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

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
