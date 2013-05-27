/* Include -------------------------------------------------------------------*/
#include "stm32f10x_it.h"

/* Variable exports-----------------------------------------------------------*/
// UART1 Messages buffer from stm32f10x_it.c
extern char RxBuffer[RxBufferSize];
extern volatile unsigned char RxCounter;
extern volatile unsigned char RTCFlag;

/* Private functions ---------------------------------------------------------*/
void RTC_SetTimeBySerial(void);
void RTC_Configuration(void);
uint32_t Time_Regulate(void);
void Time_Adjust(void);
void Time_Display(uint32_t TimeVar);
uint8_t USART_Scanf(uint32_t value);