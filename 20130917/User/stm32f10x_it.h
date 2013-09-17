/**
  ******************************************************************************
  * @file    SysTick/TimeBase/stm32f10x_it.h 
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   This file contains the headers of the interrupt handlers.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F10x_IT_H
#define __STM32F10x_IT_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

/* -- USART1 message buffersize -- */
#define RxBufferSize   0x32
/* ------------------------------- */

// GPS Character buffer maximum size
#define MaxSize 0x80

/* Private typedef -----------------------------------------------------------*/
typedef struct {
  volatile int data_x[100];
  volatile int data_y[100];
  volatile int data_z[100];
  volatile int data_g[100];
  volatile int ATFCBit[100];
}AXISDATA;

typedef struct {
  volatile int tmp_data_x_lcd[100];
  volatile int tmp_data_y_lcd[100];
  volatile int tmp_data_z_lcd[100];
  volatile int ATFCBit_lcd[100];
}AXISBUF;

/* Private define ------------------------------------------------------------*/
#define SELECT_AXIS_X   0
#define SELECT_AXIS_Y   1
#define SELECT_AXIS_Z   2

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
void delay_ms(__IO uint32_t nTime);
void delay_us(__IO uint32_t nTime);
void TimingDelay_Decrement(void);
void TIM2_IRQHandler(void);
void USART1_IRQHandler(void);
void USART3_IRQHandler(void);
void EXTI1_IRQHandler(void);
void EXTI2_IRQHandler(void);
void EXTI4_IRQHandler(void);
void EXTI9_5_IRQHandler(void);

#endif /* __STM32F10x_IT_H */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
