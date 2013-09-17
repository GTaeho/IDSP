/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "rtc.h"
#include "calendar.h"
#include "clcd.h"
#include "usb_type.h"
#include <stdio.h>

/* Private define ------------------------------------------------------------*/
#define RTCClockOutput_Enable  /* RTC Clock/64 is output on tamper pin(PC.13) */

/* Private variables ---------------------------------------------------------*/
// Date and Time
char DATE[16];
char TIME[16];

/* Date and Time variable updates every second */
uint32_t TYR = 0, TMO = 0, TDD = 0;
__IO uint32_t THH = 0, TMM = 0, TSS = 0;

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
    
    /* RTC Configuration */
    RTC_Configuration();
    
    printf("\r\n * Please set calendar");
    
    /* setup year, month, date in 4, 2, 2 digits each */
    Date_Regulate();
    
    printf("\r\n * Please set time");

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
    
    /* Check whether we've written year or month or day value before */
    if(BKP_ReadBackupRegister(BKP_DR2) == 0x0000 || BKP_ReadBackupRegister(BKP_DR3) == 0x0000) {
      /* setup year, month, date in 4, 2, 2 digits each */
      Date_Regulate();
    } else {
      
      uint16_t YY, MD;
      YY = BKP_ReadBackupRegister(BKP_DR2);
      MD = BKP_ReadBackupRegister(BKP_DR3);
      
      int month, day;
      if( (MD / 1000) % 10 == 0) {
        month = (MD / 100) % 10;
      } else {
        month = (MD / 1000) % 10 + (MD / 100) % 10 ;
      }
      
      if( (MD / 10) % 10 == 0 ) {
        day = MD % 10;
      } else {
        day = MD - (MD / 100) * 100;
      }
      
      printf("\r\n\n Previous written calendar data found !");
      printf("\r\n Written values are as follows :");
      printf("\r\n Year : %d, Month : %d, Day : %d", YY, month, day);
      printf("\r\n Above calendar datas will be used to set current calendar automatically\r\n");
      
      TranslateIntoYear(YY);
      TranslateIntoMonth(month);
      TranslateIntoDay(day);
    }
        
    /* NVIC MUST BE CONFIGURED before branch into power on reset */
    NVIC_InitTypeDef NVIC_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
  
    /* Configure one bit for preemption priority */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  
    /* Enable the RTC Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    NVIC_InitStructure.NVIC_IRQChannel = RTCAlarm_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0xFF;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    /* Configure EXTI Line17 (RTC Alarm)to generate an interrupt on rising edge */
    EXTI_ClearITPendingBit(EXTI_Line17);
    EXTI_InitStructure.EXTI_Line = EXTI_Line17;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_Init(&EXTI_InitStructure);
  
    /* Wait for RTC registers synchronization */
    RTC_WaitForSynchro();
    
    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask();
    
    /* Alarm in 3 second */
    //RTC_SetAlarm(3);
    
    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask();
    
    /* Enable the RTC Second, RTC Alarm interrupt */
    RTC_ITConfig(RTC_IT_SEC || RTC_IT_ALR, ENABLE);
    
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
  
  /* Clear reset flags */
  RCC_ClearFlag();
}

/**
  * @brief  Configures the RTC.
  * @param  None
  * @retval None
  */
void RTC_Configuration(void) {
  NVIC_InitTypeDef NVIC_InitStructure;
  EXTI_InitTypeDef EXTI_InitStructure;
  
  /* Configure one bit for preemption priority */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

  /* Enable the RTC Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  /* 2 bits for Preemption Priority and 2 bits for Sub Priority */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

  NVIC_InitStructure.NVIC_IRQChannel = RTCAlarm_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0xFF;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  /* Configure EXTI Line17 (RTC Alarm)to generate an interrupt on rising edge */
  EXTI_ClearITPendingBit(EXTI_Line17);
  EXTI_InitStructure.EXTI_Line = EXTI_Line17;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_Init(&EXTI_InitStructure);
  
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
  
  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForLastTask();
  
  /* Alarm in 3 second */
  //RTC_SetAlarm(3);

  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForLastTask();
  
  /* Enable the RTC Second, RTC Alarm interrupt */
  RTC_ITConfig(RTC_IT_SEC || RTC_IT_ALR, ENABLE);

  /* Set RTC prescaler: set RTC period to 1sec */
  RTC_SetPrescaler(32767); /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) */

  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForLastTask();
}

/**
  * @brief  Returns the date entered by user, using Hyperterminal.
  * @param  None
  * @retval Current time RTC counter value
  */
uint32_t Date_Regulate(void) {
  int MMDD;
  uint32_t Tmp_YY = 0xFF, Tmp_MM = 0xFF, Tmp_DD = 0xFF;

  printf("\r\n================= Date Setting ==================");
  printf("\r\n  Please Set Year in 4 digits : ");

  while (Tmp_YY == 0xFF) {
    Tmp_YY = USART_4DigitScanf(9999);
  }
  printf("  Set year complete!");
  printf("\r\n  Please Set Month : ");
  while (Tmp_MM == 0xFF) {
    Tmp_MM = USART_2DigitScanf(12);
  }
  printf("  Set month complete!");
  printf("\r\n  Please Set Day : ");
  while (Tmp_DD == 0xFF) {
    Tmp_DD = USART_2DigitScanf(31);
  }
  
  TranslateIntoYear(Tmp_YY);
  TranslateIntoMonth(Tmp_MM);
  TranslateIntoDay(Tmp_DD);
  
  MMDD = (Tmp_MM * 100) + Tmp_DD;
  
  /* Save year data to unresettable backup register addr. no. 2, 3 */
  BKP_WriteBackupRegister(BKP_DR2, Tmp_YY); // Save Year
  BKP_WriteBackupRegister(BKP_DR3, MMDD); // Save Month and Date  
  
  printf("  Set day complete!");

  /* return 0 if succeed */
  return 0;
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
    Tmp_HH = USART_2DigitScanf(23);
  }
  printf("  Set hour complete!");
  printf("\r\n  Please Set Minutes : ");
  while (Tmp_MM == 0xFF) {
    Tmp_MM = USART_2DigitScanf(59);
  }
  printf("  Set minutes complete!");
  printf("\r\n  Please Set Seconds : ");
  while (Tmp_SS == 0xFF) {
    Tmp_SS = USART_2DigitScanf(59);
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
void Time_Adjust(void) {
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
void Time_Display(uint32_t TimeVar) { 
  /* Reset RTC Counter when Time is 23:59:59 */
  if (RTC_GetCounter() == 0x0001517F) {
    RTC_SetCounter(0x0);
    IncreaseSingleDay();
    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask();
    
    /* Backup routine*/
    int YY, MD;
    YY = GetYearAndMergeToInt();
    MD = GetMonthAndMergeToInt() * 100 + GetDayAndMergeToInt();
    
    printf("\r\n\n Automatic Backup routine excuted");
    
    BKP_WriteBackupRegister(BKP_DR2, YY);
    BKP_WriteBackupRegister(BKP_DR3, MD);
    
    printf("\r\n Calendar data successfully saved to backup register BKP_DR2, 3");
  }
  
  /* get calendar */
  TYR = GetYearAndMergeToInt();
  TMO = GetMonthAndMergeToInt();
  TDD = GetDayAndMergeToInt();
  
  /* Compute  hours */
  THH = TimeVar / 3600;
  /* Compute minutes */
  TMM = (TimeVar % 3600) / 60;
  /* Compute seconds */
  TSS = (TimeVar % 3600) % 60;
  
  int mTHH, mTMM, mTSS;
  mTHH = THH; mTMM = TMM; mTSS = TSS;

  sprintf(DATE, "DATE: %04d.%02d.%02d", TYR, TMO, TDD);
  sprintf(TIME, "TIME:   %02d:%02d:%02d", mTHH, mTMM, mTSS);
  
  /* Display through CLCD */
  CLCD_Write(0, 0, DATE);
  CLCD_Write(0, 1, TIME);
}

/**
  * @brief  Gets numeric values from the hyperterminal.
  * @param  None
  * @retval None
  */
uint32_t USART_4DigitScanf(uint32_t value) {
  uint32_t index = 0;
  uint32_t tmp[4] = {0, 0, 0, 0};

  while(index < 4) {
    /* loop untill valid input */
    while(!RTCFlag);
    RTCFlag = false;  // this should happen once
    tmp[index++] = RxBuffer[RxCounter - 1];
    if ((tmp[index - 1] < 0x30) || (tmp[index - 1] > 0x39)) {
      printf("\r\nPlease enter valid number between 0 and 9");
      index--;
    }
  }
  /* Calculate the Corresponding value */
  index = ((tmp[0] - 0x30) * 1000) + ((tmp[1] - 0x30) * 100) + ((tmp[2] - 0x30) * 10) + ((tmp[3] - 0x30) * 1);
  /* Checks */
  if (index > value) {
    printf("\r\nPlease enter valid number between 0 and %d", value);
    return 0xFF;
  }
  
  return index;
}

/**
  * @brief  Gets numeric values from the hyperterminal.
  * @param  None
  * @retval None
  */
uint32_t USART_3DigitScanf(uint32_t value) {
  uint32_t index = 0;
  uint32_t tmp[3] = {0, 0, 0};

  while(index < 3) {
    /* loop untill valid input */
    while(!RTCFlag);
    RTCFlag = false;  // this should happen once
    tmp[index++] = RxBuffer[RxCounter - 1];
    if ((tmp[index - 1] < 0x30) || (tmp[index - 1] > 0x39)) {
      printf("\r\nPlease enter valid number between 0 and 9");
      index--;
    }
  }
  /* Calculate the Corresponding value */
  index = ((tmp[0] - 0x30) * 100) + ((tmp[1] - 0x30) * 10) + ((tmp[2] - 0x30) * 1);
  /* Checks */
  if (index > value) {
    printf("\r\nPlease enter valid number between 0 and %d", value);
    return 0xFF;
  }
  
  return index;
}

/**
  * @brief  Gets numeric values from the hyperterminal.
  * @param  None
  * @retval None
  */
uint8_t USART_2DigitScanf(uint32_t value) {
  uint32_t index = 0;
  uint32_t tmp[2] = {0, 0};

  while(index < 2) {
    /* loop untill valid input */
    while(!RTCFlag);
    RTCFlag = false;  // this should happen once
    tmp[index++] = RxBuffer[RxCounter - 1];
    if ((tmp[index - 1] < 0x30) || (tmp[index - 1] > 0x39)) {
      printf("\r\nPlease enter valid number between 0 and 9");
      index--;
    }
  }
  /* Calculate the Corresponding value */
  index = ((tmp[0] - 0x30) * 10) + (tmp[1] - 0x30);
  /* Checks */
  if (index > value) {
    printf("\r\nPlease enter valid number between 0 and %d", value);
    return 0xFF;
  }
  
  return index;
}

/**
  * @brief  Gets numeric values from the hyperterminal.
  * @param  None
  * @retval None
  */
uint8_t USART_1DigitScanf(uint32_t value) {
  uint32_t index = 0;
  uint32_t tmp[1] = {0};

  while(index < 1) {
    /* loop untill valid input */
    while(!RTCFlag);
    RTCFlag = false;  // this should happen once
    tmp[index++] = RxBuffer[RxCounter - 1];
    if ((tmp[index - 1] < 0x30) || (tmp[index - 1] > 0x39)) {
      printf("\r\nPlease enter valid number between 0 and 9");
      index--;
    }
  }
  /* Calculate the Corresponding value */
  index = tmp[0] - 0x30;
  /* Checks */
  if (index > value) {
    printf("\r\nPlease enter valid number between 0 and %d", value);
    return 0xFF;
  }
  
  return index;
}

