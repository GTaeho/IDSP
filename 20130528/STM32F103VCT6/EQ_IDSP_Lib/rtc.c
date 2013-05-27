/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "rtc.h"
#include "clcd.h"
#include "usb_type.h"
#include <stdio.h>

/* Private define ------------------------------------------------------------*/
#define RTCClockOutput_Enable  /* RTC Clock/64 is output on tamper pin(PC.13) */

/* Private variables ---------------------------------------------------------*/
// Date and Time
char Time[16];
char Date[16];

/* Time variable updates every second */
uint32_t THH = 0, TMM = 0, TSS = 0;

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Setting up the time by Serial USART1.
  * @param  None
  * @retval None
  */
void RTC_SetTimeBySerial(void) {
  if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5) {
    /* Backup data register value is not correct or not yet programmed (when
       the first time the program is executed) */

    printf("\r\n * Begin RTC initialization");
    printf("\r\n * Please set time. This only happen once and for all");
    
    /* RTC Configuration */
    RTC_Configuration();

    /* Adjust time by values entered by the user on the hyperterminal */
    Time_Adjust();
    
    BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
  } else {
    /* Check if the Power On Reset flag is set */
    if (RCC_GetFlagStatus(RCC_FLAG_PORRST) != RESET) {
      printf("\r\n * Power On Reset occurred....");
    }
    /* Check if the Pin Reset flag is set */  
    else if (RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET) {
      printf("\r\n\n * External Reset occurred....");
    }

    printf("\r\n * No need to configure RTC....");
    /* Wait for RTC registers synchronization */
    RTC_WaitForSynchro();

    /* Enable the RTC Second */
    RTC_ITConfig(RTC_IT_SEC, ENABLE);
    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask();
  }

#ifdef RTCClockOutput_Enable
  /* Enable PWR and BKP clocks */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

  /* Allow access to BKP Domain */
  PWR_BackupAccessCmd(ENABLE);

  /* Disable the Tamper Pin */
  BKP_TamperPinCmd(DISABLE); /* To output RTCCLK/64 on Tamper pin, the tamper
                                 functionality must be disabled */

  /* Enable RTC Clock Output on Tamper Pin */
  BKP_RTCOutputConfig(BKP_RTCOutputSource_Second);
#endif
}

/**
  * @brief  Configures the RTC.
  * @param  None
  * @retval None
  */
void RTC_Configuration(void) {
  NVIC_InitTypeDef NVIC_InitStructure;
  
  /* Configure one bit for preemption priority */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

  /* Enable the RTC Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  /* Enable PWR and BKP clocks */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

  /* Allow access to BKP Domain */
  PWR_BackupAccessCmd(ENABLE);

  /* Reset Backup Domain */
  BKP_DeInit();

  /* Enable LSE */
  RCC_LSEConfig(RCC_LSE_ON);
  /* Wait till LSE is ready */
  while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);

  /* Select LSE as RTC Clock Source */
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

  /* Enable RTC Clock */
  RCC_RTCCLKCmd(ENABLE);

  /* Wait for RTC registers synchronization */
  RTC_WaitForSynchro();

  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForLastTask();

  /* Enable the RTC Second */
  RTC_ITConfig(RTC_IT_SEC, ENABLE);

  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForLastTask();

  /* Set RTC prescaler: set RTC period to 1sec */
  RTC_SetPrescaler(32767); /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) */

  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForLastTask();
}

/**
  * @brief  Returns the time entered by user, using Hyperterminal.
  * @param  None
  * @retval Current time RTC counter value
  */
uint32_t Time_Regulate(void) {
  uint32_t Tmp_HH = 0xFF, Tmp_MM = 0xFF, Tmp_SS = 0xFF;

  printf("\r\n================= Time Setting ==================");
  printf("\r\n  Please Set Hours in 24hr : ");

  while (Tmp_HH == 0xFF) {
    Tmp_HH = USART_Scanf(23);
  }
  printf("  Set hour complete!");
  printf("\r\n  Please Set Minutes : ");
  while (Tmp_MM == 0xFF)
  {
    Tmp_MM = USART_Scanf(59);
  }
  printf("  Set minutes complete!");
  printf("\r\n  Please Set Seconds : ");
  while (Tmp_SS == 0xFF)
  {
    Tmp_SS = USART_Scanf(59);
  }
  printf("  Set second complete!");

  /* Return the value to store in RTC counter register */
  return((Tmp_HH*3600 + Tmp_MM*60 + Tmp_SS));
}

/**
  * @brief  Adjusts time.
  * @param  None
  * @retval None
  */
void Time_Adjust(void)
{
  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForLastTask();
  /* Change the current time */
  RTC_SetCounter(Time_Regulate());
  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForLastTask();
}

/**
  * @brief  Displays the current time.
  * @param  TimeVar: RTC counter value.
  * @retval None
  */
void Time_Display(uint32_t TimeVar)
{ 
  /* Reset RTC Counter when Time is 23:59:59 */
  if (RTC_GetCounter() == 0x0001517F) {
     RTC_SetCounter(0x0);
     /* Wait until last write operation on RTC registers has finished */
     RTC_WaitForLastTask();
  }
  
  /* Compute  hours */
  THH = TimeVar / 3600;
  /* Compute minutes */
  TMM = (TimeVar % 3600) / 60;
  /* Compute seconds */
  TSS = (TimeVar % 3600) % 60;

  char TIME[30];
  sprintf(TIME, "%d:%d:%d", THH, TMM, TSS);
  
  /* Display through CLCD */
  CLCD_Write(0, 0, "-= RTC  CLOCK =-");
  CLCD_Write(0, 1, TIME);
}

/**
  * @brief  Gets numeric values from the hyperterminal.
  * @param  None
  * @retval None
  */
uint8_t USART_Scanf(uint32_t value) {
  uint32_t index = 0;
  uint32_t tmp[2] = {0, 0};

  while(index < 2) {
    /* loop untill valid input */
    while(!RTCFlag);
    RTCFlag = false;  // this happen once
    tmp[index++] = RxBuffer[RxCounter-1];
    if ((tmp[index - 1] < 0x30) || (tmp[index - 1] > 0x39)) {
      printf("\r\nPlease enter valid number between 0 and 9");
      index--;
    }
  }
  /* Calculate the Corresponding value */
  index = (tmp[1] - 0x30) + ((tmp[0] - 0x30) * 10);
  /* Checks */
  if (index > value) {
    printf("\r\nPlease enter valid number between 0 and %d", value);
    return 0xFF;
  }
  
  return index;
}

