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
char DATA1_BUF[19500] = "";
char DATA2_BUF[19500] = "";

char FOOTER[] = "==========================================================\r\n"
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
  
  /* TIM2 Configuration */
  TIM2_Configuration();
  
/* Timer 3 dedicate to server source */  
#ifdef USE_EQDAS_SERVER
  
  /* TIM3 Configuration */
  TIM3_Configuration();
  
#endif
  
  /* UART1 Configuration */
  UART_Configuration();
  
  /* System Tick Configuration at 1us */
  SysTick_Config(SystemCoreClock / 1000000);
  
  /* WIZ820io SPI1 configuration */
  WIZ820io_SPI1_Configuration();
  
  /* W5200 Configuration */
  Set_network();
  
  /* EXTI Configuration */
  EXTI_Configuration();
  
  /* RTC configuration by setting the time by Serial USART1 */
  //RTC_SetTimeBySerial();
  
  /* CLCD Configuration */
  CLCD_Configuration();
  
  /* GLCD Configuration */
  GLCD_Configuration();
  
  /* FatFS configuration */
  f_mount(0, &fs);
  
  /* Display Total size of SD card in MB scale */
  SD_TotalSize();
  
  /* MAL configuration */
  Set_System();
  
  /* UMS configuration */
  Set_USBClock();
  USB_Interrupts_Config();
  USB_Init();
  
  // For TCP client's connection request delay
  presentTime = my_time;
  
  // When everything is set, print message
  printf("\r\n - System is ready - ");
  
  while (1) {
    if(TimerCount >= 1000) {  // 1000 = 1 sec
      TimerCount = 0;
      
      //my_time++;  // uncomment when tcp connection is needed
    }
    
#ifdef USE_EQDAS_SERVER
    
    if(TenMilliSecCount >= 10) {  // 10 equals 10 millisecond
      TenMilliSecCount = 0;
      
      /* PC Client Parsing routine ------------------------------------------------- */
      /* Set PCFlag indicate that we have valid connection from PC Client(port 7070) */
      if(PCFlag) {
        //PCFlag = false;
        
        char PC_Buf[20] = "";
        sprintf(PC_Buf, "X : %s, Y : %s, Z : %s\n",
                DAQBoardOne[arrIdx].AxisX,
                DAQBoardOne[arrIdx].AxisY,
                DAQBoardOne[arrIdx].AxisZ);
        // code for stacking algorithm which will combine data from two boards into one
        // Only when socket is established, allow send data
        if(getSn_SR(SOCK_TWO) == SOCK_ESTABLISHED) {
          /* send selected data */
          send(SOCK_TWO, (uint8_t*)PC_Buf, strlen(PC_Buf), (bool)false);
        }
        
        if(arrIdx < 99) arrIdx++;
        else arrIdx = 0;
      }
    }
    
#endif
    
    if(ParseUSART1) {
      ParseUSART1 = false;
      
      // print system configuration
      //printSysCfg();
      
      // run some test on SDIO
      //SDIO_TEST();

#if (defined USE_EQDAS01) || (defined USE_EQDAS02)
      /*
      int x, y, z;
      x = mAxisBuf.tmp_data_x_lcd[arrIdx];
      y = mAxisBuf.tmp_data_y_lcd[arrIdx];
      z = mAxisBuf.tmp_data_z_lcd[arrIdx];
      
      char buffer[20];
      sprintf(buffer, "------------------\n%+05d,%+05d,%+05d\n", x, y, z);
      printf("\r\nbuffer : %s", buffer);
      */
      
      /*
      strcpy(COMPLETEFILE, HEADER);
      strcat(COMPLETEFILE, DATA_BUF);
      strcat(COMPLETEFILE, FOOTER);
      printf("\r\nstrlen(COMPLETEFILE) : %d, %s", strlen(COMPLETEFILE), COMPLETEFILE);
      
      printf("\r\nscan_files(path) : ");
      res = scan_files(path);
      FPrintFatResult(res);
      
      char *dirPath = "/20130517";
      res = f_mkdir(dirPath);
      
      FPrintFatResult(res);
      
      dirPath = "/20130517/22H-23H";
      res = f_mkdir(dirPath);
      
      FPrintFatResult(res);
      
      char *filePath = "/20130517/22H-23H/test.txt";
      // Create log file on the drive 0
      res = f_open(&fsrc, filePath, FA_CREATE_ALWAYS | FA_WRITE);
      
      FPrintFatResult(res);
      
      if(res == FR_OK) {
        // Write buffer to file
        res = f_write(&fsrc, COMPLETEFILE, sizeof(COMPLETEFILE), &br);
        
        printf("\r\test.txt successfully created\r\n");
        
        // Close file
        f_close(&fsrc);
        
      } else if ( res == FR_EXIST ) {
        printf("\r\ntest.txt already exist");
      }
      */
      
#elif defined USE_EQDAS_SERVERs
      
      char *x, *y, *z;
      x = DAQBoardOne[arrIdx].AxisX;
      y = DAQBoardOne[arrIdx].AxisY;
      z = DAQBoardOne[arrIdx].AxisX;
      
      char buffer[20];
      sprintf(buffer, "X : %s, Y : %s, Z : %s\n", x, y, z);
      //printf("\r\nRX_BUF : %s, strlen(RX_BUF) : %d", (char*)RX_BUF, strlen((char*)RX_BUF));
      printf("RX_BUF = %s", RX_BUF);
      
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
    
    if(flag_uart == 1) {
      tmp_start = index;
    }
    
    /* Choose Axis and Generate waveform */
    switch(mode) {
    case SELECT_AXIS_X :
      //SelectAxisX();
      break;
    case SELECT_AXIS_Y : 
      //SelectAxisY();
      break;
    case SELECT_AXIS_Z : 
      //SelectAxisZ();
      break;
    }
    
#if (defined USE_EQDAS01) || (defined USE_EQDAS02)
    
    // On every impulse out of 100Hz do the work
    if(RbitFlag) {
      RbitFlag = false;
      
      arrIdx = index;
      
      // copy to buffer
      mAxisBuf.tmp_data_x_lcd[arrIdx] = mAxisData.data_x[arrIdx];
      mAxisBuf.tmp_data_y_lcd[arrIdx] = mAxisData.data_y[arrIdx];
      mAxisBuf.tmp_data_z_lcd[arrIdx] = mAxisData.data_z[arrIdx];
      
      // Copy to Temporary GAL array
      CopyToTmpGalArray(arrIdx);
      
      // Cut off to 1G
      CutOffTo1G(arrIdx);
      
      // Calculate GAL and copy to single temporary GAL value
      CalculateGalAndCopyToGal(arrIdx);
      
      // Determine KMA scale
      DetermineKMA(arrIdx);
      
      // Check sign bit and apply to int container
      CheckSignAndToInt(arrIdx);
      
      // Copy to data buffer to be written through FATFS
      CopyToFatFsDataBuffer(arrIdx);   
      
      /* EQDAQ01, 02 Client Routine ---------------------------------------------*/
      /* E1Flag or E2Flag set when client board successfully connect to server --*/
      /* Refer to wiz820.c line no. 300 for which flag to be set */
      if(E1Flag) {
        int x, y, z;
        x = mAxisBuf.tmp_data_x_lcd[arrIdx];
        y = mAxisBuf.tmp_data_y_lcd[arrIdx];
        z = mAxisBuf.tmp_data_z_lcd[arrIdx];
        
        char E1_Buf[20];
        sprintf(E1_Buf, "%+05d,%+05d,%+05d\r\n", x, y, z);
        // Only when socket is established, allow send data
        if(getSn_SR(SOCK_ZERO) == SOCK_ESTABLISHED) {
          /* send selected data */
          CountSendByte = send(SOCK_ZERO, (uint8_t*)E1_Buf, strlen(E1_Buf), (bool)false);
        }
      }
        
      if(E2Flag) {
        int x, y, z;
        x = mAxisBuf.tmp_data_x_lcd[arrIdx];
        y = mAxisBuf.tmp_data_y_lcd[arrIdx];
        z = mAxisBuf.tmp_data_z_lcd[arrIdx];
        
        char E2_Buf[20];
        sprintf(E2_Buf, "%+d,%+d,%+d", x, y, z);
        // Only when socket is established, allow send data
        if(getSn_SR(SOCK_ZERO) == SOCK_ESTABLISHED) {
          /* send selected data */
          send(SOCK_ZERO, (uint8_t*)E2_Buf, strlen(E2_Buf), (bool)false);
        }
      }
  
      // increase index so that we can add to next array
      index++;
    }
    
// following routine is only necessary when the board works as server
#elif defined USE_EQDAS_SERVER
      
    /* EQ-DAQ-01 Parsing routine ------------------------------------------------- */
    /* Set E1Flag indicate that we have valid connection from EQ-DAQ-01(port 5050) */
    if(E1Flag) {
      E1Flag = false; // clear flag since this routine excutes ceaselessly over time
      
      ProcessTextStream(EQ_ONE, (char*)RX_BUF, E1Order);
      
      if(E1Order < 99) E1Order++;
      else E1Order = 0;
    }
    
    /* EQ-DAQ-02 Parsing routine ------------------------------------------------- */
    /* Set E2Flag indicate that we have valid connection from EQ-DAQ-02(port 6060) */
    if(E2Flag) {
      E2Flag = false;
      
      ProcessTextStream(EQ_TWO, (char*)RX_BUF, E2Order);
      
      if(E2Order < 99) E2Order++;
      else E2Order = 0;
    }

#endif

/* Setup TCP Client or Server -----------------------------------------------------*/
/* Please open config.h file to choose a proper board you wish to use */    
#if (defined USE_EQDAS01) || (defined USE_EQDAS02)    

      /* Start TCP Client process */
      ProcessTcpClient(SOCK_ZERO);
    
#elif defined USE_EQDAS_SERVER
    
      /* Process server socket with each port */
      ProcessTcpServer(SOCK_ZERO, 5050);  // designated as for EQM-DAQ-01 with port 5050
      ProcessTcpServer(SOCK_ONE, 6060);   // designated as for EQM-DAQ-02 with port 6060
      ProcessTcpServer(SOCK_TWO, 7070);   // designated as for PC-CLIENT  with port 7070
      ProcessTcpServer(SOCK_THREE, 8080); // designated as for PC_DUMP    with port 8080
      
      /* Socket 4 to 7 reserved for future application
       * ProcessTcpServer(SOCK_FOUR, 9090);   // designated as for TOBEUSED with port 9090
       * ProcessTcpServer(SOCK_FIVE, 10010);   // designated as for TOBEUSED with port 10010
       * ProcessTcpServer(SOCK_SIX, 10020);    // designated as for TOBEUSED with port 10020
       * ProcessTcpServer(SOCK_SEVEN, 10030);  // designated as for TOBEUSED with port 10030
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

