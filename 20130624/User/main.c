/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

// DEBUG
int CountSendByte = 0;

char HEADER[] = "==========================================================\r\n"
                   " * FILE NAME : TEST.TXT\r\n"
                   " * FILE TYPE : SEISMIC ACTIVITY DATA LOG\r\n"
                   " * CURRENT DATE : 2013.05.18\r\n"
                   " * CURRENT TIME : 02H.23M.00S.00\r\n"
                   " * GPS CALIBRATED : NO\r\n"
                   " * DUMPED : NO\r\n"
                   "==========================================================\r\n";

/* Two data buffer big enough to contain 5 seconds of data */
char DATA1_BUF[18500] = "";
char DATA2_BUF[18500] = "";

char FOOTER[] = "\r\n==========================================================\r\n"
                   " * END OF FILE\r\n"
                   "==========================================================";

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

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
  
  /* System Clocks Configuration */
  RCC_Configuration();
  
  /* System Tick Configuration at 1us */
  SysTick_Config(SystemCoreClock / 1000000);
  
#if (defined USE_EQDAS01) || (defined USE_EQDAS02)
  
  /* TIM2 Configuration */
  TIM2_Configuration();
  
  /* TIM3 Configuration (10ms for Network Transmission)*/
  TIM3_Configuration();
  
  /* TIM5 Configuration (GLCD Time Share)*/
  TIM5_Configuration();
  
  /* TIM6 Configuration (RTC load) */
  TIM6_Configuration();
  
#elif defined USE_EQDAS_SERVER
  
  /* TIM4 Configuration */
  TIM4_Configuration();
  
#endif
  
  /* CLCD Configuration */
  CLCD_Configuration();
  
  /* GLCD Configuration */
  GLCD_Configuration();
  
  /* UART1 Configuration */
  UART_Configuration();
  
  /* GPS-UART3 Configuration */
  GPS_Configuration();
  
  /* RTC configuration by setting the time by Serial USART1 */
  RTC_SetTimeBySerial();
  
  /* Forcefully let user set the IP through terminal */
  //ForceIPSetBySerial();

  /* WIZ820io SPI1 configuration */
  WIZ820io_SPI1_Configuration();
  
  /* W5200 Configuration */
  Set_network();
  
  /* Print WIZ820io configuration */
  printSysCfg();
  
  /* EXTI Configuration */
  EXTI_Configuration();
  
  /* FatFS configuration */
  f_mount(0, &fs);  // mSD
  //f_mount(1, &fs);  // NAND
  
  /* Display Total size of SD card in MB scale */
  SD_TotalSize();
  
  /* Scan all files in mSD card */
  scan_files(path);
  
  /* MAL configuration */
  //Set_System();
  
  /* UMS configuration */
  //Set_USBClock();
  //USB_Interrupts_Config();
  //USB_Init();
  
  /* loop upon completion of USB Enumeration */
  //while (bDeviceState != CONFIGURED);
  
  /* ATFC Algorithm GPIO */
  ATFC_GPIO_Configuration();
  
  /* ATFC Parameter Initialization */
  ATFCAlgorithmParameterSetup();
  
  // For TCP client's connection request delay
  presentTime = my_time;
  
  /* Clear CLCD before branch into main */
  CLCD_Clear();

  /* Create directory and sub directory in accordance with current date */
  filePath = CreateDirectoryAccordingly(GetYearAndMergeToInt(), GetMonthAndMergeToInt(), 
                                        GetDayAndMergeToInt(), RTC_GetCounter() / 3600);
  
  /* Create file in append mode in accordance with current minute */
  CreateFileAppendModeAccordingly(filePath, (RTC_GetCounter() % 3600) / 60);
  
  // When everything is set, print message
  printf("\r\n\n - System is ready - ");
  
  while (1) {
    
#if (defined USE_EQDAS01) || (defined USE_EQDAS02)
    
    if(TimerCount > 1000) { // 0 ~ 999 (1000) = 1 sec
      TimerCount = 0;
      
      /*
      int x, y, z;
      x = mAxisBuf.tmp_data_x_lcd[index];
      y = mAxisBuf.tmp_data_y_lcd[index];
      z = mAxisBuf.tmp_data_z_lcd[index];
    
      char Serial_Buf[37];
      int hour, minute, second, tmsecond;
      hour = THH; minute = TMM; second = TSS; tmsecond = 0;
      sprintf(Serial_Buf, "%04d%02d%02d_%02d%02d%02d%02d_%+05d_%+05d_%+05d\r\n", 
                year, month, day, hour, minute, second, tmsecond, x, y, z);
      printf(Serial_Buf);
      */
      
      my_time++;  // uncomment when tcp connection is needed
      
      USART_SendData(USART3, 'H');
      
      /* Process Parameter Text Stream */
      if(PCFlag) {  // EQDAS Client System and ATFC Algorithm Setting
        PCFlag = false;
        
        ProcessParameterStream();
      }
    }
    
    /* GLCD Time Share */
    if(TIM5GLCDCount > 95) {  // Allocate proper time to share mcu resource with network
      TIM5GLCDCount = 0;
      
      /* Graphic LCD Copy and Process Routine -------------------------------------- */
      if(SyncFlag) { // prevent unpleasant impuse
        // Index synchronization dedicate to GLCD
        arrGLCDIdx = index;
        
        // Make a copy from raw collected data to temporary array
        CopyToTmpArray(arrGLCDIdx);
          
        // Copy to Temporary GAL array
        //CopyToTmpGalArray(arrIdx);
        
        // Calculate GAL and copy to single temporary GAL value
        //CalculateGalAndCopyToGal(arrIdx);
        
        // Determine KMA scale
        KMAGrade = DetermineKMA(arrGLCDIdx);
    
        // Check sign bit and apply to int container
        CheckSignAndToInt(arrGLCDIdx); // this function also cuts surplus 1G
      }
      
      int mATFCBit_lcd;
      mATFCBit_lcd = mAxisBuf.ATFCBit_lcd[arrGLCDIdx];
      
      /* Switch menu & waveform display through graphic lcd */
      GLCD_AxisViewWithWaveform(mode, arrGLCDIdx);
      
      /* Display KMA Intensity on Graphic LCD */
      GLCD_DisplayKMAIntensity(KMAGrade, mATFCBit_lcd);
    }
    
    /* Clock generated by TIM3 */
    if(ClientTimerCounter > 10) {  // 0 ~ 999 (1000) = 1 sec
      ClientTimerCounter = 0;
      
      year = GetYearAndMergeToInt();
      month = GetMonthAndMergeToInt();
      day = GetDayAndMergeToInt();
      hour = THH; minute = TMM; second = TSS; tmsecond = 0;
      
      int mYear, mMonth, mDay, mHour, mMin, mSec, mTMSec;
      mYear = year; mMonth = month; mDay = day;
      mHour = hour; mMin = minute; mSec = second; mTMSec = tmsecond;

      if(SyncFlag) {
        // Index synchronization
        arrIdx = index;
      
        // Make a copy from raw collected data to temporary net array
        CopyToNetArray(arrIdx);
        
        // Check sign bit and apply to int container
        CheckSignAndToIntForNet(arrIdx); // this function also cuts surplus 1G
      }
      
      /* Prevent access to volatile variable warning */
      /* This have to be here in order to correct data to be used in ATFC */
      int mX, mY, mZ, mATFCBit;
      mX = mAxisNetBuf.axis_x_for_net[arrIdx];
      mY = mAxisNetBuf.axis_y_for_net[arrIdx];
      mZ = mAxisNetBuf.axis_z_for_net[arrIdx];
      mATFCBit = mAxisNetBuf.ATFCBit_net[arrIdx];
      
      // Copy to data buffer to be written through FATFS
      //CopyToFatFsDataBuffer(arrIdx);
      
      /* EQDAQ01, 02 Client Routine ---------------------------------------------*/
      /* E1Flag or E2Flag set when client board successfully connect to server --*/
      /* Refer to wiz820.c line no. 406 for which flag to be set */
      if(E1Flag) {
        char E1_Buf[45];
        sprintf(E1_Buf, "%04d%02d%02d_%02d%02d%02d%02d_%+05d_%+05d_%+05d_%d\r\n", 
                mYear, mMonth, mDay, mHour, mMin, mSec, mTMSec,
                mX, mY, mZ, mATFCBit);
        // Only when socket is established, allow send data
        if(getSn_SR(SOCK_ZERO) == SOCK_ESTABLISHED) {
          /* send selected data */
          CountSendByte = send(SOCK_ZERO, (uint8_t*)E1_Buf, strlen(E1_Buf), (bool)false);
        }
      }
      
      if(E2Flag) {
        char E2_Buf[45];
        sprintf(E2_Buf, "%04d%02d%02d_%02d%02d%02d%02d_%+05d_%+05d_%+05d_%d\r\n", 
                mYear, mMonth, mDay, mHour, mMin, mSec, mTMSec,
                mX, mY, mZ, mATFCBit);
        // Only when socket is established, allow send data
        if(getSn_SR(SOCK_ZERO) == SOCK_ESTABLISHED) {
          /* send selected data */
          send(SOCK_ZERO, (uint8_t*)E2_Buf, strlen(E2_Buf), (bool)false);
        }
      }
    }
    
    /* do RTC work on every second */
    if(RTCTIM6Count > 1000) {
      RTCTIM6Count = 0;
      
      if(InitialThirteenSeconds == 12) {
         InitialThirteenSeconds = 0;
         GoATFCFlag = true;
      } else {
        if(!GoATFCFlag) {
           InitialThirteenSeconds++;
        }
      }
      
      /* RTC 1Hz interrupt */
      if(RTCTimeDisplay) { // 1Hz calibrated by RTC
        RTCTimeDisplay = false;
        
        /* Adjust realtime clock deviation */
        if(hour > 23) {
          int i, currentDay, mDay, mHour, mMin, mSec;
          mDay = hour / 24;
          
          for(i=0; i<mDay; i++) {
            IncreaseSingleDay();
            if(i == mDay - 1) {
              IncreaseSingleDay();
              currentDay = (GetMonthAndMergeToInt() * 100) + GetDayAndMergeToInt();
              BKP_WriteBackupRegister(BKP_DR3, currentDay); // Save Month and Date
            }
          }
          
          mHour = THH % 24;
          mMin = TMM;
          mSec = TSS;
          
          /* Change the current time */
          RTC_SetCounter(mHour*3600 + mMin*60 + mSec);
        }
        
        /* Display current time */
        Time_Display(RTC_GetCounter());
      }
      
      /* RTC Alarm interrupt */
      if(RTCAlarmFlag) {
        RTCAlarmFlag = false;
        
        printf("\r\nRTC Alarm Actviated!");
      } 
    }
    
    /* Save log to file process ------------------------------------------------*/
    /* Save process needs to be run every single cycle due to delay might occur */
    if(GoAppendDataFlag) {  // every 500 sample (equals 5 sec), go save file.
      GoAppendDataFlag = false;
      
      int bytesWritten = 0;
      
      if(EachSecFlag) {
        // it means that DATA1_BUF is full and ready to flush out
        // be sure to empty out DATA1_BUF or will overflow and cause system to halt.
        
        /* Append first data for the duration of 1 second */
        bytesWritten = f_printf(&fsrc, DATA1_BUF);
        printf("\r\n%d of bytesWritten", bytesWritten);
        
        if(FileRecordCompleteFlag) {
          FileRecordCompleteFlag = false;
          printf("\r\nFile Record Complete!");
          /* Close the file */
          f_close(&fsrc);
        }
        
        // Reset DATA1_BUF
        memset(DATA1_BUF, 0, sizeof(DATA1_BUF)); 
      } else {
        /* Append another second of data */
        bytesWritten = f_printf(&fsrc, DATA2_BUF);
        printf("\r\n%d of bytesWritten", bytesWritten);
        
        if(FileRecordCompleteFlag) {
          FileRecordCompleteFlag = false;
          printf("\r\nFile Record Complete!");
          /* Close the file */
          f_close(&fsrc); 
        }
        
        // Reset DATA2_BUF
        memset(DATA2_BUF, 0, sizeof(DATA2_BUF));
      }
    }
    
#endif
    
    if(ParseGPS) {
      ParseGPS = false;
      
      printf("%s", GPS_Buffer);
    }
    
    if(ParseUSART1) {
      ParseUSART1 = false;
      
      // run some test on SDIO
      //SDIO_TEST();

#if (defined USE_EQDAS01) || (defined USE_EQDAS02)
      
      /* Print WIZ820io configuration */
      printSysCfg();

      printf("\r\n");
      printf("\r\nBKP_DR1 = %d", BKP_ReadBackupRegister(BKP_DR1));
      printf("\r\nBKP_DR2 = %d", BKP_ReadBackupRegister(BKP_DR2));
      printf("\r\nBKP_DR3 = %d", BKP_ReadBackupRegister(BKP_DR3));
      printf("\r\nBKP_DR4 = %d", BKP_ReadBackupRegister(BKP_DR4));
      printf("\r\nBKP_DR5 = %d", BKP_ReadBackupRegister(BKP_DR5));
      printf("\r\nBKP_DR6 = %d", BKP_ReadBackupRegister(BKP_DR6));
      printf("\r\nBKP_DR7 = %d", BKP_ReadBackupRegister(BKP_DR7));
      printf("\r\nBKP_DR8 = %d", BKP_ReadBackupRegister(BKP_DR8));
      printf("\r\nBKP_DR9 = %d", BKP_ReadBackupRegister(BKP_DR9));
      printf("\r\nBKP_DR10 = %d", BKP_ReadBackupRegister(BKP_DR10));
      printf("\r\nBKP_DR11 = %d", BKP_ReadBackupRegister(BKP_DR11));
      printf("\r\nBKP_DR12 = %d", BKP_ReadBackupRegister(BKP_DR12));
      printf("\r\nBKP_DR13 = %d", BKP_ReadBackupRegister(BKP_DR13));
      printf("\r\nBKP_DR14 = %d", BKP_ReadBackupRegister(BKP_DR14));
      printf("\r\nBKP_DR15 = %d", BKP_ReadBackupRegister(BKP_DR15));
      printf("\r\nBKP_DR16 = %d", BKP_ReadBackupRegister(BKP_DR16));
      
      printf("\r\n\r\nUSART_ReceiveData(USART3) = %d", USART_ReceiveData(USART3));
      
#elif (defined) USE_EQDAS_SERVER
      
      char buffer[37];
      sprintf(buffer, "%s_%s_%s_%s_%s\r\n",
                DAQBoardOne[arrIdx].Date,
                DAQBoardOne[arrIdx].Time,
                DAQBoardOne[arrIdx].AxisX,
                DAQBoardOne[arrIdx].AxisY,
                DAQBoardOne[arrIdx].AxisZ);
      printf("\r\nRX_BUF : %s, strlen(RX_BUF) : %d", (char*)RX_BUF, strlen((char*)RX_BUF));
      printf("\r\nstrlen(buffer) = %d\n%s", strlen(buffer), buffer);
      
      /*
      char *original = "-3843,+4095,+2069";
      char target[20];
      strncpy(target, original, strlen(original));
      
      char *one, *two, *three;
      char *AfterToken;
      AfterToken = strtok(target, ",");
      one = AfterToken;
      AfterToken = strtok(NULL, ",");
      two = AfterToken;
      AfterToken = strtok(NULL, ",");
      three = AfterToken;
      AfterToken = strtok(NULL, ",");
      if(AfterToken != NULL) printf("AfterToken is not empty");
      
      printf("\r\none : %s, two : %s, three : %s", one, two, three);*/
      
#endif
    
    }
    
// following routine is only necessary when the board works as server
#if defined USE_EQDAS_SERVER
    
    /* Server also needs to have get CLCD going while running */
    /* RTC 1Hz interrupt */
    if(RTCTimeDisplay) { // 1Hz calibrated by RTC
      RTCTimeDisplay = false;
      
      /* Adjust realtime clock deviation */
      if(hour > 23) {
        int i, currentDay, mDay, mHour, mMin, mSec;
        mDay = hour / 24;
        
        for(i=0; i<mDay; i++) {
          IncreaseSingleDay();
          if(i == mDay - 1) {
            IncreaseSingleDay();
            currentDay = (GetMonthAndMergeToInt() * 100) + GetDayAndMergeToInt();
            BKP_WriteBackupRegister(BKP_DR3, currentDay); // Save Month and Date
          }
        }
        
        mHour = THH % 24;
        mMin = TMM;
        mSec = TSS;
        
        /* Change the current time */
        RTC_SetCounter(mHour*3600 + mMin*60 + mSec);
      }
      
      /* Display current time */
      Time_Display(RTC_GetCounter());
    }

    /* When multiple daq board send data to server board, follow different suit */
    if(E1Flag && E2Flag) {
       E1Flag = false; E2Flag = false;
       
       // Save incoming message to structure array pointer character
      ProcessTextStream(EQ_ONE, (char*)RX_BUF, E1Order);
      
      // Save incoming message to structure array pointer character
      ProcessTextStream(EQ_TWO, (char*)RX_BUF, E2Order);

      /* PC Client Parsing routine ------------------------------------------------- */
      /* Set PCFlag indicate that we have valid connection from PC Client(port 7070) */
      if(PCFlag) {
        // Send directly to PC
        MultipleBoardDataToSendToPC(EQ_BOTH, E1Order, E2Order);
      }
      
      if(E1Order < 100) E1Order++;
      else E1Order = 0;
      
      if(E2Order < 100) E2Order++;
      else E2Order = 0;
    }

#endif

/* Setup TCP Client or Server -----------------------------------------------------*/
/* Please open config.h file to choose a proper board you wish to use -------------*/    
#if (defined USE_EQDAS01) || (defined USE_EQDAS02)

      /* Start TCP Client process */
      ProcessTcpClient(SOCK_ZERO);  // TCP Client
      
      /* Parameter setting Server side with port 5050 in default */
      ATFCTcpServer(SOCK_TWO, EQDAS_Conf_PORT);  // SOCK_TWO because of flag conformity
    
#elif defined USE_EQDAS_SERVER
    
      /* Process server socket with each port */
      ProcessTcpServer(SOCK_ZERO, 5050);  // designated as for EQM-DAQ-01 with port 5050
      ProcessTcpServer(SOCK_ONE, 6060);   // designated as for EQM-DAQ-02 with port 6060
      ProcessTcpServer(SOCK_TWO, 7070);   // designated as for PC-CLIENT  with port 7070
      ProcessTcpServer(SOCK_THREE, 8080); // designated as for PC_DUMP    with port 8080
      
      /*
      ProcessTcpServer(SOCK_FOUR, 9090);   // designated as for TOBEUSED with port 9090
      ProcessTcpServer(SOCK_FIVE, 10010);   // designated as for TOBEUSED with port 10010
      ProcessTcpServer(SOCK_SIX, 10020);    // designated as for TOBEUSED with port 10020
      ProcessTcpServer(SOCK_SEVEN, 10030);  // designated as for TOBEUSED with port 10030
      */
      
#endif
      
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
  while (1);
}

#endif

/**
  * @}
  */

/**
  * @}
  */

/************************ END OF FILE ************************/

