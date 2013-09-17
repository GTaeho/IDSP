/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/*
static const char HEADER[] = 
"==========================================================\r\n"
" * FILE NAME : TEST.TXT\r\n"
" * FILE TYPE : SEISMIC ACTIVITY DATA LOG\r\n"
" * CURRENT DATE : 2013.05.18\r\n"
" * CURRENT TIME : 02H.23M.00S.00\r\n"
" * GPS CALIBRATED : NO\r\n"
" * DUMPED : NO\r\n"
"==========================================================\r\n";
*/

/*
static const char FOOTER[] = 
"\r\n==========================================================\r\n"
" * END OF FILE\r\n"
"==========================================================";
*/

static int ThirtyMinuteMark;
static bool ThirtyMinuteFlag;

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
  
  /* TIM2 Configuration (Client & ATFC Server) */
  TIM2_Configuration();
  
#elif defined USE_EQDAS_SERVER
  
  /* TIM4 Configuration */
  TIM4_Configuration();
  
#endif
  
  /* TIM5 Configuration (GLCD & Ethernet) */
  TIM5_Configuration();
  
  /* TIM6 Configuration (RTC load) */
  //TIM6_Configuration();
  
  /* CLCD Configuration */
  CLCD_Configuration();
  
  /* GLCD Configuration */
  GLCD_Configuration();
  
  /* UART1 Configuration */
  UART_Configuration();
  
  /* RTC configuration by setting the time by Serial USART1 */
  RTC_SetTimeBySerial();
  
  /* Let user set the IP through terminal forcefully */
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
  //scan_files(path);
  
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
  
  /* GPS-UART3 Configuration - This have to be here otherwise it wouldn't work */
  GPS_Configuration();
  
  // For TCP client's connection request delay
  presentTime = my_time;

  /* Create directory and sub directory in accordance with current date */
  filePath = CreateDirectoryAccordingly(GetYearAndMergeToInt(), GetMonthAndMergeToInt(), 
                                        GetDayAndMergeToInt(), RTC_GetCounter() / 3600);
  
  /* Create file in append mode in accordance with current minute */
  CreateFileAppendModeAccordingly(filePath, (RTC_GetCounter() % 3600) / 60);
  
  /* Clear GLCD to better represent waveform */
  GLCD_Clear();
  
  //BKP_WriteBackupRegister(BKP_DR8, 0);
  
  // When everything is set, print message
  printf("\r\n\n - System is ready - ");
  
  while (1) {
    
#if (defined USE_EQDAS01) || (defined USE_EQDAS02)
    
    /* Index synchronization routine -----------------------------------------------*/
    if(SyncFlag) { // prevent unpleasant impuse from happening
      // Index synchronization dedicated to GLCD & Ethernet
      if(arrIdx != index) {
        // Index synchronization
        arrIdx = index;
      }
    }
    /* End of index synchronization routine -----------------------------------------*/
    
    if(TimerCount > 999) { // 0 ~ 999 (1000) = 1 sec
      TimerCount = 0;
      
      //my_time++;  // uncomment when tcp connection is needed
      
      /* Setup TCP Client or Server -------------------------------------------------*/
      /* Please open config.h file to choose a proper board you wish to use ---------*/    
      /* Start TCP Client process */
      ProcessTcpClient(SOCK_ZERO);  // TCP Client
      
      /* Parameter setting Server side with port 5050 in default */
      ATFCTcpServer(SOCK_TWO, EQDAS_Conf_PORT);  // SOCK_TWO because of flag conformity
      /*------------------------------------------------------------------------------*/
      
      /* Process Parameter Text Stream -----------------------------------------------*/
      if(PCFlag) {  // EQDAS Client System and ATFC Algorithm Setting
        PCFlag = false;
        
        ProcessParameterStream();
      }
      /* End of Parameter process ----------------------------------------------------*/
    }

    /* 10ms interval between points */
    if(TIM5Count >= 9) {
      TIM5Count = 0;
      
      // Make a copy from raw collected data to temporary array
      CopyToTmpArray(arrIdx);
      
      // Determine KMA scale
      KMAGrade = DetermineKMA(arrIdx);
      
      // Check sign bit and apply to int container
      CheckSignAndToInt(arrIdx); // this function also cuts surplus 1G
      
      /* Switch menu & waveform display through graphic lcd */
      GLCD_AxisViewWithWaveform(mode, arrIdx);
      
      //int mATFCBit;
      //mATFCBit_lcd = mAxisBuf.ATFCBit_lcd[arrIdx];
      
      int AxisDataToATFCAlgorithm, mATFCEventDetection;
      AxisDataToATFCAlgorithm = mAxisBuf.tmp_data_y_lcd[arrIdx];  // Axis Z
      ATFCAlgorithm(AxisDataToATFCAlgorithm);
      mATFCEventDetection = EventDetection;
      
      /* Display KMA Intensity on Graphic LCD */
      GLCD_DisplayKMAIntensity(KMAGrade, mATFCEventDetection);
      
      /* Prevent access to volatile variable warning */
      /* This have to be here in order to correct data to be used in ATFC */      
      /* ATFC Server side for each EQ DAS Client */
      if(EQATFCFlag) {
        int mYear, mMonth, mDay, mHour, mMin, mSec, mTMSec;
        mYear = year; mMonth = month; mDay = day;
        mHour = hour; mMin = minute; mSec = second; mTMSec = arrIdx;
        
        int mX, mY, mZ, mATFCBit;
        mX = mAxisBuf.tmp_data_x_lcd[arrIdx] >> 2;
        mY = mAxisBuf.tmp_data_y_lcd[arrIdx] >> 2;
        mZ = mAxisBuf.tmp_data_z_lcd[arrIdx] >> 2;
        mATFCBit = mATFCEventDetection;
        
        char ATFC_Buf[40];
        sprintf(ATFC_Buf, "%04d%02d%02d_%02d%02d%02d%02d_%+05d_%+05d_%+05d_%d\r\n",
                mYear, mMonth, mDay, mHour, mMin, mSec, mTMSec,
                mX, mY, mZ, mATFCBit);
        // Only when socket is established, allow send data
        if(getSn_SR(SOCK_TWO) == SOCK_ESTABLISHED) {  // SOCK_TWO : PC
          /* send selected data */
          send(SOCK_TWO, (uint8_t*)ATFC_Buf, strlen(ATFC_Buf), (bool)false);
        }
      }
      // Copy to data buffer to be written through FATFS
      //CopyToFatFsDataBuffer(arrIdx);
    }
    
    /* RTC 1Hz interrupt */
    if(RTCTimeDisplay) { // 1Hz calibrated by RTC
      RTCTimeDisplay = false;
      
      int TimeVar;
      TimeVar = RTC_GetCounter();
      /* Compute hour */
      THH = TimeVar / 3600;
      /* Compute minute */
      TMM = (TimeVar % 3600) / 60;
      /* Compute second */
      TSS = (TimeVar % 3600) % 60;
      
      /* Refresh date on every 1s */
      year = GetYearAndMergeToInt();
      month = GetMonthAndMergeToInt();
      day = GetDayAndMergeToInt();
      hour = THH; minute = TMM; second = TSS; tmsecond = 0;
      
      if(ThirtyMinuteMark == 1799) {
        ThirtyMinuteMark = 0;
        ThirtyMinuteFlag = true;
      } else {
        ThirtyMinuteMark++; 
      }
      
      /* Adjust realtime clock deviation */
      if(hour > 23) {
        int i, currentDay, mDay, mHour, mMin, mSec;
        mDay = hour / 24;
        
        for(i=0; i<mDay; i++) {
          IncreaseSingleDay();
          if(i == mDay - 1) {
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
    }
    
#endif
    
    if(ParseGPS) {
      ParseGPS = false;
      
      char *srcstr = "$GPRMC";
      char *token = ",";  char *processedString;
      char StringYear[3], StringMonth[3], StringDay[3], StringHour[3], StringMinute[3], StringSecond[3];
      int GPSYear, GPSMonth, GPSDay, GPSHour, GPSMinute, GPSSecond;
      if(strncmp((char const*)GPS_Buffer, srcstr, 6) == 0) {
        //printf("GPS_Buffer = %s\r\n\r\n", (char*)GPS_Buffer);
        processedString = strtok((char*)GPS_Buffer, token);
        processedString = strtok(NULL, token);
        
        strncpy(StringHour, processedString, 2); StringHour[2] = 0;
        strncpy(StringMinute, processedString+2, 2); StringMinute[2] = 0;
        strncpy(StringSecond, processedString+4, 2); StringSecond[2] = 0;
        
        GPSHour = atoi(StringHour) + 9; // Current Hour = StringHour + 9
        GPSMinute = atoi(StringMinute);
        GPSSecond = atoi(StringSecond);
        
        int i; for(i=4; i!=0 ; i--) processedString = strtok(NULL, token);
        
        strncpy(StringYear, processedString+4, 2); StringYear[2] = 0;
        strncpy(StringMonth, processedString+2, 2); StringMonth[2] = 0;
        strncpy(StringDay, processedString, 2); StringDay[2] = 0;
        
        GPSYear = atoi(StringYear) + 2000;  // Currnet Year = StringYear + 2000
        GPSMonth = atoi(StringMonth);
        GPSDay = atoi(StringDay);
        
        /* The Year is chosen as criteria to the time */
        if( (GPSYear == GetYearAndMergeToInt()) && ThirtyMinuteFlag ) { // only when year matches between RTC and GPS
          ThirtyMinuteFlag = false;
          
          if(GPSMonth != GetMonthAndMergeToInt() || GPSDay != GetDayAndMergeToInt() ||
             GPSHour != THH || GPSMinute != TMM || GPSSecond != TSS) {
            /* Change the month and day */
            TranslateIntoMonth(GPSMonth);
            TranslateIntoDay(GPSDay);
            
            /* Save year data to unresettable backup register addr. no. 3 */
            int MMDD; MMDD = (GPSMonth * 100) + GPSDay;
            BKP_WriteBackupRegister(BKP_DR3, MMDD); // Save Month and Date  
            
            /* Change the current time */
            RTC_SetCounter(GPSHour*3600 + GPSMinute*60 + GPSSecond);
            
            printf("GPSHour = %d\r\n", GPSHour);
            printf("GPSMinute = %d\r\n", GPSMinute);
            printf("GPSSecond = %d\r\n\r\n", GPSSecond);
            
            printf("GPSYear = %d\r\n", GPSYear);
            printf("GPSMonth = %d\r\n", GPSMonth);
            printf("GPSDay = %d\r\n\r\n", GPSDay);
            
            printf("GPS-to-System synchronization complete!\r\n\r\n");
          }
        }
      }
    }
    
    if(ParseUSART1) {
      ParseUSART1 = false;
      
      // run some test on SDIO
      //SDIO_TEST();

#if (defined USE_EQDAS01) || (defined USE_EQDAS02)
      
      /* Print WIZ820io configuration */
      printSysCfg();

      printf("\r\n");
      printf("BKP_DR1 = %d\r\n", BKP_ReadBackupRegister(BKP_DR1));
      printf("BKP_DR2 = %d\r\n", BKP_ReadBackupRegister(BKP_DR2));
      printf("BKP_DR3 = %d\r\n", BKP_ReadBackupRegister(BKP_DR3));
      printf("BKP_DR4 = %d\r\n", BKP_ReadBackupRegister(BKP_DR4));
      printf("BKP_DR5 = %d\r\n", BKP_ReadBackupRegister(BKP_DR5));
      printf("BKP_DR6 = %d\r\n", BKP_ReadBackupRegister(BKP_DR6));
      printf("BKP_DR7 = %d\r\n", BKP_ReadBackupRegister(BKP_DR7));
      printf("BKP_DR8 = %d\r\n", BKP_ReadBackupRegister(BKP_DR8));
      printf("BKP_DR9 = %d\r\n", BKP_ReadBackupRegister(BKP_DR9));
      printf("BKP_DR10 = %d\r\n", BKP_ReadBackupRegister(BKP_DR10));
      printf("BKP_DR11 = %d\r\n", BKP_ReadBackupRegister(BKP_DR11));
      printf("BKP_DR12 = %d\r\n", BKP_ReadBackupRegister(BKP_DR12));
      printf("BKP_DR13 = %d\r\n", BKP_ReadBackupRegister(BKP_DR13));
      printf("BKP_DR14 = %d\r\n", BKP_ReadBackupRegister(BKP_DR14));
      printf("BKP_DR15 = %d\r\n", BKP_ReadBackupRegister(BKP_DR15));
      printf("BKP_DR16 = %d\r\n\r\n", BKP_ReadBackupRegister(BKP_DR16));
      
      printf("RX_BUF = %s\r\n", RX_BUF);
      
      /*
      printf("\r\nstrlen(HEADER) : %d %s", strlen(HEADER), HEADER);
      
      printf("\r\nf_mkdir1 : ");
      char *dirPath = "0:/20130517";
      res = f_mkdir(dirPath);
      FPrintFatResult(res);
      
      printf("\r\nf_mkdir2 : ");
      dirPath = "0:/20130517/22H-23H";
      res = f_mkdir(dirPath);
      FPrintFatResult(res);
      
      char *filePath = "0:/20130517/2-23H/test.txt";
      // Create log file on the drive 0
      res = open_append(&fsrc, filePath);
      FPrintFatResult(res);
      
      if(res == FR_OK) {
        printf("test.txt successfully created\r\n");
        
        // Write buffer to file
        int bytesWritten;
        bytesWritten = f_printf(&fsrc, HEADER);
        printf("\r\n%d of bytesWritten", bytesWritten);
        
        // Close file
        f_close(&fsrc);
        
      } else if ( res == FR_EXIST ) {
        printf("\r\ntest.txt already exist");
      }
      */
      
      
#elif (defined) USE_EQDAS_SERVER
      
      char buffer[40];
      sprintf(buffer, "%s_%s_%s_%s_%s\r\n",
                DAQBoardOne[arrIdx].Date,
                DAQBoardOne[arrIdx].Time,
                DAQBoardOne[arrIdx].AxisX,
                DAQBoardOne[arrIdx].AxisY,
                DAQBoardOne[arrIdx].AxisZ);
      printf("\r\nRX_BUF : %s, strlen(RX_BUF) : %d", (char*)RX_BUF, strlen((char*)RX_BUF));
      printf("\r\nstrlen(buffer) = %d\n%s", strlen(buffer), buffer);
      
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
    
    /* EQ-DAQ-01 Parsing routine ------------------------------------------------- */
    /* Set E1Flag indicate that we have valid connection from EQ-DAQ-01(port 5050) */
    if(E1Flag) {
      E1Flag = false; // clear flag since this routine excutes ceaselessly over time
      
      ProcessTextStream(EQ_ONE, (char*)RX_BUF, E1Order);
      
      /* PC Client Parsing routine ------------------------------------------------- */
      /* Set PCFlag indicate that we have valid connection from PC Client(port 7070) */
      if(PCFlag && !E2Flag) { // only when PC is connected and EQ-DAQ-02 is not connected
        // Send directly to PC
        SingleBoardDataToSendToPC(EQ_ONE, E1Order-10);
      }
      
      if(E1Order < 99) E1Order++;
      else E1Order = 0;
    }
    
    /* EQ-DAQ-02 Parsing routine ------------------------------------------------- */
    /* Set E2Flag indicate that we have valid connection from EQ-DAQ-02(port 6060) */
    if(E2Flag) {
      E2Flag = false;
      
      ProcessTextStream(EQ_TWO, (char*)RX_BUF, E2Order);
      
      /* PC Client Parsing routine ------------------------------------------------- */
      /* Set PCFlag indicate that we have valid connection from PC Client(port 7070) */
      if(PCFlag && !E1Flag) { // only when PC is connected and EQ-DAQ-01 is not connected
        // Send directly to PC
        //SendToPC(EQ_TWO, E2Order);
      }
      
      if(E2Order < 99) E2Order++;
      else E2Order = 0;
      
      /* PC Client Parsing routine ------------------------------------------------- */
      /* Set PCFlag indicate that we have valid connection from PC Client(port 7070) */
      if(PCFlag) {
        // Send directly to PC
        MultipleBoardDataToSendToPC(EQ_BOTH, E1Order-10, E2Order-10);
      }
    }
    
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

