/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F4xx_IT_H
#define __STM32F4xx_IT_H

#ifdef __cplusplus
 extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"

/* Private define ------------------------------------------------------------*/
/* -- UART4 message buffersize -- */
#define RxBufferSize   0x32
/* ------------------------------- */

/* Private typedef -----------------------------------------------------------*/
typedef struct {
  signed short data_x[100];
  signed short data_y[100];
  signed short data_z[100];
  signed short data_g[100];
}AXISDATA;

typedef struct {
  signed short tmp_data_x_lcd[100];
  signed short tmp_data_y_lcd[100];
  signed short tmp_data_z_lcd[100];
}AXISBUF;

typedef struct {
  signed short tmp_gal_x[100];
  signed short tmp_gal_y[100];
  signed short tmp_gal_z[100];
}GALBUF;

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
void TIM4_IRQHandler(void);
void UART4_IRQHandler(void);
void RTC_WKUP_IRQHandler(void);
void RTC_Alarm_IRQHandler(void);
void EXTI4_IRQHandler(void);
void EXTI9_5_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_IT_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
