/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "stm32f4xx_it.h"
#include "wiz820io.h"
#include "socket.h"
#include "w5200.h"
#include "tlcd.h"
#include "rtc.h"
#include "glcd.h"
#include "exti.h"
#include "myAccel3LV02.h"
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define SELECT_AXIS_X   0
#define SELECT_AXIS_Y   1
#define SELECT_AXIS_Z   2

/* Private macro -------------------------------------------------------------*/
/* Socket Enumeration ---*/
enum { SOCK_ZERO };
/* ----------------------*/

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

// GLCD display data from glcd.c
extern unsigned char plot_x[72];
extern unsigned char plot_y[72];
extern unsigned char plot_z[72];

// GLCD Graph container from stm32f4xx_it.c
extern unsigned char mode, i, j, x, y, offset_start, offset_finish;
extern int index, start, flag_uart, tmp_start;
extern int data_x[100], data_y[100], data_z[100], data_g[100];
extern int tmp_data_x_lcd[100], tmp_data_y_lcd[100], tmp_data_z_lcd[100];

// myAccel3LV02 Axis data
uint16_t Xdata, Ydata, Zdata;

// axis order variable
uint8_t order = 0;

// Time criteria
uint32_t my_time;
uint32_t presentTime;

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
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
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
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

/* RETARGET PRINTF */
int fputc(int ch, FILE *stream) {
  while(USART_GetFlagStatus(UART4, USART_FLAG_TXE) == RESET);
  
  USART_SendData(UART4, (char) ch);

  return ch;
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
  
  /* WIZ820io SPI2 configuration */
  WIZ820io_SPI2_Configuration();  // one that is on left one
  
  /* WIZ820io SPI3 configuration */
  //WIZ820io_SPI3_Configuration();  // one that is on right one

  /* W5200 Configuration */
  Set_network();
  
  /* TLCD configuration */
  TLCD_Configuration();
  
  //TLCD_Write(0, 0, str1);
  //TLCD_Write(0, 1, str2);
  
  /* RTC Configuration */
  RTC_Configuration();
  
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
  
  // For TCP client's connection request delay
  presentTime = my_time;
  
  // When everything is set, print message
  printf("\r\n - System is ready - ");
    
  while(1) {
    if(TimerCount >= 1000) {  // thousand equals one second
      TimerCount = 0;
      
      // for Calculate connection delay
      my_time++;
      
      // retrieve axis data
      GetAccelValue(AXIS_X, &Xdata);
      GetAccelValue(AXIS_Y, &Ydata);
      GetAccelValue(AXIS_Z, &Zdata);
      
      char str[30];
      sprintf(str, "%d,%d,%d", 0xFFF&Xdata, 0xFFF&Ydata, 0xFFF&Zdata);
      TLCD_Clear();
      TLCD_Write(0, 0, str);
    }
    
    if(ParseUART4) {
      ParseUART4 = False; 
      
      // print Wiz810io configuration
      printSysCfg();
    }
    
    if(flag_uart == 1) {
      tmp_start = start;
    }
        
    switch(mode) {
    case SELECT_AXIS_X : break;
    case SELECT_AXIS_Y : break;
    case SELECT_AXIS_Z : break;
    }
    
    /* Ethernet Client Routine -----------------------------------------------*/
    /* SendFlag get set from when socket established and received any message */
    if(SendFlag) {
      SendFlag = False;
      
      char AxisData[30];
      for(order = 0; order < 100 ; order++) {
        float EW = 0;
        float NS = 0;
        float UD = 0;
        
        EW = data_x[order] * 1e-7;
        NS = data_y[order] * 1e-7;
        UD = data_z[order] * 1e-7;
        
        sprintf(AxisData, "%-0.7f,%-0.7f,%-0.7f\n", EW, NS, UD);
        
        // Only when socket is established, send data
        if(getSn_SR(SOCK_ZERO) == SOCK_ESTABLISHED) {
          /* send the received data */
          send(SOCK_ZERO, (uint8*)AxisData, strlen(AxisData), (bool)False);
        }
      }
    }
    
    /* Process client socket with port 5050 */
    //ProcessTcpClient(SOCK_ZERO);
    
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

