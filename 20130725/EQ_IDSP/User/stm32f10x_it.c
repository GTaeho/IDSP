/**
  ******************************************************************************
  * @file    SysTick/TimeBase/stm32f10x_it.c 
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and peripherals
  *          interrupt service routine.
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
#include "stm32f10x_it.h"
#include "usb_lib.h"
#include "usb_istr.h"
#include "usb_pwr.h"
#include "rtc.h"
#include "sdcard.h"
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
// Timer variable
volatile uint16_t TimerCount = 0;
volatile int TenMilliSecCount = 0;
volatile int RTCTIM6Count = 0;
volatile int TIM5Count;

// UART1 Messages buffer
char RxBuffer[RxBufferSize];
volatile unsigned char RxCounter = 0;
volatile unsigned char ParseUSART1;
volatile unsigned char RTCFlag = false;

// GPS USART3 buffer
volatile uint8_t GPS_Buffer[MaxSize];
volatile uint8_t GPSRxCounter;
volatile bool ParseGPS;

// struct data container from stm32f10x_it.h
AXISDATA    mAxisData;
AXISBUF     mAxisBuf;

// GLCD Graph container
volatile int mode = 0;
volatile int BitCount = 15;
volatile int index = 0;
volatile bool SyncFlag;

// Prevent illegal access to volatile variable
int mIndex;

// systick variable
__IO uint32_t TimingDelay = 0;

//DEBUG
bool ExceptionFlag = true;

// RTC 1 second flag
__IO uint32_t RTCTimeDisplay = 0;

// RTC Alarm Flag
volatile int RTCAlarmFlag = false;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
  if(ExceptionFlag) {
    ExceptionFlag = false;
    printf("\r\n * NMI Exception Occured!");
  }
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void) {
  if(ExceptionFlag) {
    ExceptionFlag = false;
    printf("\r\n * HardFault Exception Occured!"); 
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  if(ExceptionFlag) {
    ExceptionFlag = false;
    printf("\r\n * Memory Manage Exception Occured!"); 
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  if(ExceptionFlag) {
    ExceptionFlag = false;
    printf("\r\n * Bus Fault Exception Occured!"); 
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  if(ExceptionFlag) {
    ExceptionFlag = false;
    printf("\r\n * Usage Fault Error Occured!"); 
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
  if(ExceptionFlag) {
    ExceptionFlag = false;
    printf("\r\n * SVCall Exception Occured!"); 
  }
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
  if(ExceptionFlag) {
    ExceptionFlag = false;
    printf("\r\n * Debug Monitor Exception Occured!"); 
  }
}

/**
  * @brief  This function handles PendSV_Handler exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
  if(ExceptionFlag) {
    ExceptionFlag = false;
    printf("\r\n * PendSV_Handler Exception Occured!"); 
  }
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
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles TIM2 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM2_IRQHandler(void) {
  if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    
    TimerCount++;
  }
}

/**
  * @brief  This function handles TIM4 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM4_IRQHandler(void) {
  if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET) {
    TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
    
    TenMilliSecCount++;
  }
}

/**
  * @brief  This function handles TIM5 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM5_IRQHandler(void) {
  if (TIM_GetITStatus(TIM5, TIM_IT_Update) != RESET) {
    TIM_ClearITPendingBit(TIM5, TIM_IT_Update);
    
    TIM5Count++;
  }
}

/**
  * @brief  This function handles TIM6 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM6_IRQHandler(void) {
  if (TIM_GetITStatus(TIM6, TIM_IT_Update) != RESET) {
    TIM_ClearITPendingBit(TIM6, TIM_IT_Update);
    
    RTCTIM6Count++;
  }
}

/**
  * @brief  This function handles USART1 global interrupt request.
  * @param  None
  * @retval None
  */
void USART1_IRQHandler(void) {
  if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
    RxBuffer[RxCounter] = USART_ReceiveData(USART1) & 0xFF;
    USART_SendData(USART1, (RxBuffer[RxCounter] & 0xFF));
    if(RxBuffer[RxCounter] == '\n') {
      while(RxBuffer[RxCounter + 1] != 0) {
        RxBuffer[RxCounter + 1] = 0;
      }
      ParseUSART1 = true;
      RxCounter = 0;
    } else {
      RxCounter++;
    }
    RTCFlag = true;
  }
}

/**
  * @brief  This function handles USART3 global interrupt request.
  * @param  None
  * @retval None
  */
void USART3_IRQHandler(void) {
  if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) {
    GPS_Buffer[GPSRxCounter] = USART_ReceiveData(USART3) & 0xFF;
    if(GPS_Buffer[GPSRxCounter] == '\n') {
      while(GPS_Buffer[GPSRxCounter + 1] != 0) {
        GPS_Buffer[GPSRxCounter + 1] = 0;
      }
      ParseGPS = true;
      GPSRxCounter = 0;
    } else {
      GPSRxCounter++;
    }
  }
}

/**
  * @brief  This function handles USB High Priority or CAN TX interrupts requests requests.
  * @param  None
  * @retval None
  */
void USB_HP_CAN1_TX_IRQHandler(void) {
  CTR_HP();
}

/**
  * @brief  This function handles USB Low Priority or CAN RX0 interrupts requests.
  * @param  None
  * @retval None
  */
void USB_LP_CAN1_RX0_IRQHandler(void) {
  USB_Istr();
}

/**
  * @brief  This function handles SDIO interrupt request.
  * @param  None
  * @retval None
  */
void SDIO_IRQHandler(void) {
  /* Process All SDIO Interrupt Sources */
  SD_ProcessIRQSrc();
}

/**
  * @brief  This function handles RTC global interrupt request.
  * @param  None
  * @retval None
  */
void RTC_IRQHandler(void) {
  if (RTC_GetITStatus(RTC_IT_SEC) != RESET) {
    /* Clear the RTC Second interrupt */
    RTC_ClearITPendingBit(RTC_IT_SEC);
    
    /* Enable time update */
    RTCTimeDisplay = true;

    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask();   
  }
}

/**
  * @brief  This function handles RTC Alarm interrupt request.
  * @param  None
  * @retval None
  */
void RTCAlarm_IRQHandler(void) {
  if(RTC_GetITStatus(RTC_IT_ALR) != RESET) {
    EXTI_ClearITPendingBit(EXTI_Line17);   /* Clear EXTI line17 pending bit */
    
    /* Clear the RTC Second interrupt */
    RTC_ClearITPendingBit(RTC_IT_ALR);

    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask(); 
    
    // code here
    RTCAlarmFlag = true;
  }
}

/**
  * @brief  This function handles EXTI1 (F_SYNC, PA01) interrupt request.
  * @param  None
  * @retval None
  */
void EXTI1_IRQHandler(void) {
  /* EXTI1(PA01 -> F_SYNC : 100Hz) */
  if(EXTI_GetITStatus(EXTI_Line1) != RESET) {
    /* Clear the  EXTI line 1 pending bit */
    EXTI_ClearITPendingBit(EXTI_Line1);    
    
    // Code to deal with F_SYNC (100Hz impulse)
    // On every impulse out of 100Hz do the work
    if(BitCount > 99) {
      BitCount = 0;
      if(index > 98) index = 0;
      else index++;
    }
  }
}

/**
  * @brief  This function handles EXTI2 (F_SCLK, PA02) interrupt request.
  * @param  None
  * @retval None
  */
void EXTI2_IRQHandler(void) {
  /* EXTI2(PA02 -> F_SCLK : 10KHz) */
  if(EXTI_GetITStatus(EXTI_Line2) != RESET) {
    /* Clear the  EXTI line 2 pending bit */
    EXTI_ClearITPendingBit(EXTI_Line2);
    
    // Code to deal with F_SCLK (10KHz Squarewave)
    /* 14bit out of 100bit used to contain each axis data */      
    if(BitCount < 14) {
      mIndex = index; // prevent illegal access to volatile variable
      
      // shift bits to be ready to receive newer incoming bit
      mAxisData.data_x[mIndex] = ((mAxisData.data_x[mIndex] << 1) & 0x3FFF);
      mAxisData.data_y[mIndex] = ((mAxisData.data_y[mIndex] << 1) & 0x3FFF);
      mAxisData.data_z[mIndex] = ((mAxisData.data_z[mIndex] << 1) & 0x3FFF);
      
      // read x, y, z data
      if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_5)) // if PB5 is set add 1 to data_x[start]
         mAxisData.data_x[mIndex] |= 1;
      if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0)) // if PB0 is set add 1 to data_y[start]
         mAxisData.data_y[mIndex] |= 1;
      if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1)) // if PB1 is set add 1 to data_z[start]
         mAxisData.data_z[mIndex] |= 1;
      if(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_10)) // if PD10 is set, ATFC detected by the FPGA
        mAxisData.ATFCBit[mIndex] = 1;  // no need to use |= operator
      SyncFlag = false;
    } else {
      // other than flag set activity, rest of 86bit is vacant so leave here blank
      SyncFlag = true;
    }
    BitCount++;
  }
}

/* User Switch Configuration */
void EXTI0_IRQHandler(void) {
  /* EXTI0 (PA00 USER SWITCH 1) */
  if(EXTI_GetITStatus(EXTI_Line0) != RESET) {
    /* Clear the  EXTI line 0 pending bit */
    EXTI_ClearITPendingBit(EXTI_Line0);
    
    if(mode == SELECT_AXIS_Z) {
      mode = SELECT_AXIS_X;
    } else {
      mode++;
    }
    
    printf("\r\nEXTI0 Triggered");
  }
}

/* User Switch Configuration */
void EXTI4_IRQHandler(void) {
  /* EXTI4 (PB04 USER SWITCH 1) */
  if(EXTI_GetITStatus(EXTI_Line4) != RESET) {
    /* Clear the  EXTI line 4 pending bit */
    EXTI_ClearITPendingBit(EXTI_Line4);
    
    //mode = SELECT_AXIS_X;
    printf("\r\nEXTI4 Triggered");
  }
}

/* User Switch Configuration */
void EXTI9_5_IRQHandler(void) {
  /* EXTI6 (PB06 USER SWITCH 2) */
  if(EXTI_GetITStatus(EXTI_Line6) != RESET) {
    /* Clear the  EXTI line 6 pending bit */
    EXTI_ClearITPendingBit(EXTI_Line6);

    //mode = SELECT_AXIS_Y;
    printf("\r\nEXTI6 Triggered");
    
  }
  
  /* EXTI7 (PB07 USER SWITCH 3) */
  if(EXTI_GetITStatus(EXTI_Line7) != RESET) {
    /* Clear the  EXTI line 7 pending bit */
    EXTI_ClearITPendingBit(EXTI_Line7);
    
    //mode = SELECT_AXIS_Z;
    printf("\r\nEXTI7 Triggered");
  }
}


/**
  * @}
  */ 

/**
  * @}
  */ 

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
