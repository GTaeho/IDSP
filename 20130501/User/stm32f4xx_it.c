/**
  ******************************************************************************
  * @file    RTC/RTC_Calendar/stm32f4xx_it.c 
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    18-January-2013
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_it.h"
#include "rtc.h"
#include "types.h"
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
// Timer counter
volatile uint16_t TimerCount = 0;

// UART4 Messages buffer from main.c
volatile uint8_t RxBuffer[RxBufferSize];
volatile uint8_t RxCounter = 0;
volatile uint8_t ParseUART4 = false;

// struct data container from stm32f4xx_it.h
AXISDATA mAxisData;
AXISBUF  mAxisBuf;

// GLCD Graph container
unsigned char mode = 0, i, j, x, y , offset_start = 0, offset_finish = 0;
int BitCount = 15, index = 0, RbitFlag = false, flag_uart = 0, tmp_start = 0;

int HENDHz = 0, TENKHZ = 0;

// systick variable
__IO uint32_t TimingDelay = 0;


/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief   This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void) {
  printf("\r\nHard Fault exception occured");
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void) {
  TimingDelay_Decrement();
}

/**
  * @brief  Inserts a delay time.
  * @param  nTime: specifies the delay time length, in milliseconds.
  * @retval None
  */
void delay_ms(__IO uint32_t nTime) {
  TimingDelay = nTime * 1000;

  while(TimingDelay != 0);
}

/**
  * @brief  Inserts a delay time.
  * @param  nTime: specifies the delay time length, in microseconds.
  * @retval None
  */
void delay_us(__IO uint32_t nTime) {
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

/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f40xx.s/startup_stm32f427x.s).                         */
/******************************************************************************/

/**
  * @brief  This function handles TIM4 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM4_IRQHandler(void) {
  if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET) {
    TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
    
    TimerCount++; 
  }
}

/**
  * @brief  This function handles UART4 global interrupt request.
  * @param  None
  * @retval None
  */
void UART4_IRQHandler(void) {
  if(USART_GetITStatus(UART4, USART_IT_RXNE) != RESET) {
    RxBuffer[RxCounter] = USART_ReceiveData(UART4) & 0xFF;
    if(RxBuffer[RxCounter] == '\n') {
      while(RxBuffer[RxCounter + 1] != 0) {
          RxBuffer[RxCounter + 1] = 0;
      }
      ParseUART4 = true;
      RxCounter = 0;
    } else {
      RxCounter++;
    }
  }
}

/**
  * @brief  This function handles RTC Wakeup global interrupt request.
  * @param  None
  * @retval None
  */
void RTC_WKUP_IRQHandler(void) {
  if(RTC_GetITStatus(RTC_IT_WUT) != RESET) {
    RTC_ClearITPendingBit(RTC_IT_WUT);
    EXTI_ClearITPendingBit(EXTI_Line22);
    
    /* Display the current time */
    RTC_TimeShow();
  }
}

/**
  * @brief  This function handles RTC Alarms interrupt request.
  * @param  None
  * @retval None
  */
void RTC_Alarm_IRQHandler(void) {
  if(RTC_GetITStatus(RTC_IT_ALRA) != RESET) {
    RTC_ClearITPendingBit(RTC_IT_ALRA);

    /* Display the current alarm */
    //RTC_AlarmShow();
    
    /* Display the current time */
    //RTC_TimeShow();
  } 
}

/**
  * @brief  This function handles mode select button exti interrupt.
  * @param  None
  * @retval None
  */
void EXTI4_IRQHandler(void) {
  if(EXTI_GetITStatus(EXTI_Line4) != RESET) {
    
    /* Clear the EXTI line 4 pending bit */
    EXTI_ClearITPendingBit(EXTI_Line4);
    
    // code to be written
  }
}

/**
  * @brief  This function handles External lines 9 to 5 interrupt request.
  * @param  EXTI5(PE05 -> F_SYNC : 100Hz), EXTI6(PE06 -> F_SCLK : 10KHz)
  * @retval None
  */
void EXTI9_5_IRQHandler(void) {
  /* EXTI5(PE05 -> F_SYNC : 100Hz) */
  if(EXTI_GetITStatus(EXTI_Line5) != RESET) {
    /* Clear the EXTI line 5 pending bit */
    EXTI_ClearITPendingBit(EXTI_Line5);
    
    if(BitCount >= 99) {
      BitCount = 0;
      if(index >= 99) {
        index = 0;
      } else {
        // On every impulse out of 100Hz do the work
        // index increases inside while routine
        RbitFlag = true;
      }
    }
    
    HENDHz++;
  }
  
  /* EXTI6(PE06 -> F_SCLK : 10KHz) */
  if(EXTI_GetITStatus(EXTI_Line6) != RESET) {
    /* Clear the EXTI line 6 pending bit */
    EXTI_ClearITPendingBit(EXTI_Line6);
    
    /* 14bit out of 100bit used to contain each axis data */
    if(BitCount < 14) {
      // shift bits so that be ready to receive newer incoming bit
      mAxisData.data_x[index] = ((mAxisData.data_x[index] << 1) & 0x3FFF);
      mAxisData.data_y[index] = ((mAxisData.data_y[index] << 1) & 0x3FFF);
      mAxisData.data_z[index] = ((mAxisData.data_z[index] << 1) & 0x3FFF);
      
      // read x, y, z data
      if(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_3)) // if PD3 is set add 1 to data_x[start]
         mAxisData.data_x[index] |= 1;
      if(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_4)) // if PD4 is set add 1 to data_y[start]
         mAxisData.data_y[index] |= 1;
      if(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_5)) // if PD5 is set add 1 to data_z[start]
         mAxisData.data_z[index] |= 1;
      flag_uart = 0;
    } else {
      // other than flag set activity, rest of 86bit is vacant so leave here blank
      flag_uart = 1;
    }
    // increase BitCount entire time
    BitCount++;
    TENKHZ++;
  }
}

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
