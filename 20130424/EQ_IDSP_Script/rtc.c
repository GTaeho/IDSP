/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "tlcd.h"
#include <stdio.h>

/* Private define ------------------------------------------------------------*/
/* Uncomment the corresponding line to select the RTC Clock source */
#define RTC_CLOCK_SOURCE_LSE          /* LSE used as RTC source clock */
/* #define RTC_CLOCK_SOURCE_LSI */    /* LSI used as RTC source clock. The RTC Clock
                                         may varies due to LSI frequency dispersion. */

/* Private variables ---------------------------------------------------------*/
// RTC variable
__IO uint32_t uwAsynchPrediv = 0;
__IO uint32_t uwSynchPrediv = 0;

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Configure the RTC peripheral by selecting the clock source.
  * @param  None
  * @retval None
  */
void RTC_Config(void) {
  RTC_DateTypeDef RTC_DateStructure;
  RTC_TimeTypeDef  RTC_TimeStructure;
  RTC_InitTypeDef  RTC_InitStructure;
  RTC_AlarmTypeDef RTC_AlarmStructure;
  
  /* Enable the PWR clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

  /* Allow access to RTC */
  PWR_BackupAccessCmd(ENABLE);
    
#if defined (RTC_CLOCK_SOURCE_LSI)  /* LSI used as RTC source clock*/
  /* The RTC Clock may varies due to LSI frequency dispersion */   
  /* Enable the LSI OSC */ 
  RCC_LSICmd(ENABLE);

  /* Wait till LSI is ready */  
  while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);

  /* Select the RTC Clock Source */
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
  
  /* ck_spre(1Hz) = RTCCLK(LSI) /(uwAsynchPrediv + 1)*(uwSynchPrediv + 1)*/
  uwSynchPrediv = 0xFF;
  uwAsynchPrediv = 0x7F;
  
  printf("\r\n * LSI used as RTC source clock");

#elif defined (RTC_CLOCK_SOURCE_LSE) /* LSE used as RTC source clock */
  /* Enable the LSE OSC */
  RCC_LSEConfig(RCC_LSE_ON);

  /* Wait till LSE is ready */  
  while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);

  /* Select the RTC Clock Source */
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
  /* ck_spre(1Hz) = RTCCLK(LSE) /(uwAsynchPrediv + 1)*(uwSynchPrediv + 1)*/
  uwSynchPrediv = 0xFF;
  uwAsynchPrediv = 0x7F;
  
  printf("\r\n * LSE used as RTC source clock");

#else
  #error Please select the RTC Clock source inside the main.c file
#endif /* RTC_CLOCK_SOURCE_LSI */
  
  /* Enable the RTC Clock */
  RCC_RTCCLKCmd(ENABLE);

  /* Wait for RTC APB registers synchronisation */
  RTC_WaitForSynchro();
  
  /* Configure the RTC data register and RTC prescaler */
  RTC_InitStructure.RTC_AsynchPrediv = uwAsynchPrediv;
  RTC_InitStructure.RTC_SynchPrediv = uwSynchPrediv;
  RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
  RTC_Init(&RTC_InitStructure);
  
  /* Set the alarm 05h:20min:30s */
  RTC_AlarmStructure.RTC_AlarmTime.RTC_H12     = RTC_H12_AM;
  RTC_AlarmStructure.RTC_AlarmTime.RTC_Hours   = 0x05;
  RTC_AlarmStructure.RTC_AlarmTime.RTC_Minutes = 0x20;
  RTC_AlarmStructure.RTC_AlarmTime.RTC_Seconds = 0x30;
  RTC_AlarmStructure.RTC_AlarmDateWeekDay = 0x31;
  RTC_AlarmStructure.RTC_AlarmDateWeekDaySel = RTC_AlarmDateWeekDaySel_Date;
  RTC_AlarmStructure.RTC_AlarmMask = RTC_AlarmMask_DateWeekDay;
  
  /* Configure the RTC Alarm A register */
  RTC_SetAlarm(RTC_Format_BCD, RTC_Alarm_A, &RTC_AlarmStructure);
  
  /* Configure the RTC WakeUp Clock source: CK_SPRE (1Hz) */
  RTC_WakeUpClockConfig(RTC_WakeUpClock_CK_SPRE_16bits);
  RTC_SetWakeUpCounter(0x0);
  
  /* Enable RTC Wakeup and RTC Alarm A Interrupt */
  RTC_ITConfig(RTC_IT_WUT | RTC_IT_ALRA, ENABLE);
  
  /* Enable Wakeup Counter */
  RTC_WakeUpCmd(ENABLE);
  
  /* Enable the alarm */
  RTC_AlarmCmd(RTC_Alarm_A, ENABLE);
  
  RTC_ClearFlag(RTC_FLAG_ALRAF);
  
  /* Set the date: Saturday April 20th 2013 */
  RTC_DateStructure.RTC_Year = 0x13;
  RTC_DateStructure.RTC_Month = RTC_Month_April;
  RTC_DateStructure.RTC_Date = 0x14;
  RTC_DateStructure.RTC_WeekDay = RTC_Weekday_Saturday;
  RTC_SetDate(RTC_Format_BCD, &RTC_DateStructure);
  
  /* Set the time to 00h 00mn 00s AM */
  RTC_TimeStructure.RTC_H12     = RTC_H12_AM;
  RTC_TimeStructure.RTC_Hours   = 0x00;
  RTC_TimeStructure.RTC_Minutes = 0x00;
  RTC_TimeStructure.RTC_Seconds = 0x00; 
  
  RTC_SetTime(RTC_Format_BCD, &RTC_TimeStructure);   
  
  /* Indicator for the RTC configuration */
  RTC_WriteBackupRegister(RTC_BKP_DR0, 0x32F2);
}

/**
  * @brief  Display the current time.
  * @param  None
  * @retval None
  */
void RTC_TimeShow(void) {
  RTC_TimeTypeDef  RTC_TimeStructure;
  
  uint8_t showtime[50] = {0};
  /* Get the current Time */
  RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
  
  /* Display time Format : hh:mm:ss */
  sprintf((char*)showtime,"%0.2d:%0.2d:%0.2d",
          RTC_TimeStructure.RTC_Hours,
          RTC_TimeStructure.RTC_Minutes,
          RTC_TimeStructure.RTC_Seconds);
  
  printf("\r\nCurrent Time : hh:mm:ss -> %s", showtime);
  
  TLCD_Write(0, 0, (char*)showtime);
  TLCD_Write(0, 1, (char*)showtime);
}

/**
  * @brief  Display the current Alarm.
  * @param  None
  * @retval None
  */
void RTC_AlarmShow(void) {
  RTC_AlarmTypeDef RTC_AlarmStructure;
  
  uint8_t showalarm[50] = {0};
  /* Get the current Alarm */
  RTC_GetAlarm(RTC_Format_BIN, RTC_Alarm_A, &RTC_AlarmStructure);
  
  sprintf((char*)showalarm,"%0.2d:%0.2d:%0.2d", 
          RTC_AlarmStructure.RTC_AlarmTime.RTC_Hours, 
          RTC_AlarmStructure.RTC_AlarmTime.RTC_Minutes, 
          RTC_AlarmStructure.RTC_AlarmTime.RTC_Seconds);
  
  printf("\r\nDisplay alarm Format : hh:mm:ss -> %s", showalarm);
}

void RTC_Configuration(void) {
  if (RTC_ReadBackupRegister(RTC_BKP_DR0) != 0x32F2) {  
    /* RTC configuration  */
    RTC_Config();
    /* Display the RTC Time and Alarm */
    RTC_TimeShow();
    RTC_AlarmShow();
  } else {
    /* Check if the Power On Reset flag is set */
    if (RCC_GetFlagStatus(RCC_FLAG_PORRST) != RESET) {
      /* Power On Reset occurred */
      printf("\r\n * Power On Reset occurred");
      
    /* Check if the Pin Reset flag is set */
    } else if (RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET) {
      /* External Reset occurred */
      printf("\r\n * External Reset occurred");
    }
    
    /* Enable the PWR clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

    /* Allow access to RTC */
    PWR_BackupAccessCmd(ENABLE);

    /* Wait for RTC APB registers synchronisation */
    RTC_WaitForSynchro();

    /* Clear the RTC Alarm Flag */
    RTC_ClearFlag(RTC_FLAG_ALRAF);

    /* Clear the EXTI Line 17 Pending bit (Connected internally to RTC Alarm) */
    EXTI_ClearITPendingBit(EXTI_Line17);

    /* Display the RTC Time and Alarm */
    RTC_TimeShow();
    RTC_AlarmShow();
  }
  
  EXTI_InitTypeDef EXTI_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
   
  /* RTC Alarm A Interrupt Configuration */
  /* EXTI configuration *********************************************************/
  EXTI_ClearITPendingBit(EXTI_Line17);
  EXTI_InitStructure.EXTI_Line = EXTI_Line17;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);
  
  EXTI_ClearITPendingBit(EXTI_Line22);
  EXTI_InitStructure.EXTI_Line = EXTI_Line22;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);
  
  /* Enable the RTC Alarm Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = RTC_Alarm_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure); 
  
  /* Enable the RTC Wakeup Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = RTC_WKUP_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure); 
}

