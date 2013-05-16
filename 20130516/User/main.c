/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f10x_it.h"
#include "config.h"
#include "socket.h"
#include "w5200.h"
#include "wiz820io.h"
#include "hw_config.h"
#include "usb_lib.h"
#include "usb_pwr.h"
#include "sdio.h"
#include "exti.h"
#include "clcd.h"
#include "glcd.h"
#include "rtc.h"
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Text Parser delimiter */
enum { EQ_ONE, EQ_TWO, PC_Cli, PC_DUMP };

// RCC Check variable
volatile uint32_t SYSCLK_Freq;
volatile uint32_t HCLK_Freq;
volatile uint32_t PCLK1_Freq;
volatile uint32_t PCLK2_Freq;

// Time criteria
uint32_t my_time;
uint32_t presentTime;

// TimerCount from stm32f10x_it.c
extern volatile uint16_t TimerCount;
extern volatile int TenMilliSecCount;

// UART1 Messages buffer from stm32f10x_it.c
extern char RxBuffer[RxBufferSize];
extern volatile unsigned char RxCounter;
extern volatile unsigned char ParseUSART1;

// GLCD Graph container from stm32f10x_it.c
extern unsigned char mode, i, j, x, y, offset_start, offset_finish;
extern int BitCount, index, RbitFlag, flag_uart;

// GLCD graph variable
unsigned short tmp_start = 0;

// String parse flags from wiz820io.c
extern uint8_t E1Flag;
extern uint8_t E2Flag;
extern uint8_t PCFlag;

// WIZ820io TX, RX Buffer
extern uint8 TX_BUF[TX_RX_MAX_BUF_SIZE]; // TX Buffer for applications
extern uint8 RX_BUF[TX_RX_MAX_BUF_SIZE]; // RX Buffer for applications

// AxisData Buffer typedef struct from stm32f4xx_it.h
extern AXISDATA mAxisData;
extern AXISBUF  mAxisBuf;

// Array index order
int E1Order = 0, E2Order = 0;;

// for faster, efficient performance variable
int arrIdx = 0;

// export buffer to send to PC
extern EQ_DAQ_ONE DAQBoardOne[100];

// DEBUG
int CountSendByte = 0;
int COuntReadByte = 0;


/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Configures the different system clocks.
  * @param  None
  * @retval None
  */
void RCC_Configuration(void) {
  RCC_ClocksTypeDef RCC_ClockFreq;
  
  RCC_GetClocksFreq(&RCC_ClockFreq);
  
  /* Wait till HSE is ready */
  while (RCC_GetFlagStatus(RCC_FLAG_HSERDY) == RESET);
  
  SYSCLK_Freq = RCC_ClockFreq.SYSCLK_Frequency;
  HCLK_Freq = RCC_ClockFreq.HCLK_Frequency;
  PCLK1_Freq = RCC_ClockFreq.PCLK1_Frequency;
  PCLK2_Freq = RCC_ClockFreq.PCLK2_Frequency;  
}

/**
  * @brief  Configures TIM2 clock and interrupt.
  * @param  None
  * @retval None
  */
void TIM2_Configuration(void) {
  NVIC_InitTypeDef NVIC_InitStructure;
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  
  /* Prescaler value */
  uint16_t PrescalerValue = 0;
  
  /* Enable TIM2 Clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
  
  /* Enable the TIM2 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Compute the prescaler value */
  PrescalerValue = (uint16_t) (SystemCoreClock / 1000000) - 1;

  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 1000;
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

  /* Prescaler configuration */
  TIM_PrescalerConfig(TIM2, PrescalerValue, TIM_PSCReloadMode_Immediate);

  /* TIM IT enable */
  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

  /* TIM2 enable counter */
  TIM_Cmd(TIM2, ENABLE); 
}

/**
  * @brief  Configures TIM3 clock and interrupt dedicate to server.
  * @param  None
  * @retval None
  */
void TIM3_Configuration(void) {
  NVIC_InitTypeDef NVIC_InitStructure;
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  
  /* Prescaler value */
  uint16_t PrescalerValue = 0;
  
  /* Enable TIM3 Clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
  
  /* Enable the TIM3 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Compute the prescaler value */
  PrescalerValue = (uint16_t) (SystemCoreClock / 1000000) - 1;

  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 1000;
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

  /* Prescaler configuration */
  TIM_PrescalerConfig(TIM3, PrescalerValue, TIM_PSCReloadMode_Immediate);

  /* TIM IT enable */
  TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

  /* TIM3 enable counter */
  TIM_Cmd(TIM3, ENABLE); 
}

void UART_Configuration() {
  GPIO_InitTypeDef GPIO_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  
  /* Enable GPIO, USART1 Clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
  
  /* Configure USART1 Tx as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* Configure USART1 Rx as input floating */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* Enable the USART1 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  /* USART1 configuration ------------------------------------------------------*/
  /* USART1 configured as follow:
        - BaudRate = 115200 baud  
        - Word Length = 8 Bits
        - One Stop Bit
        - No parity
        - Hardware flow control disabled (RTS and CTS signals)
        - Receive and transmit enabled
  */ 
  USART_InitTypeDef USART_InitStructure;
  
  USART_InitStructure.USART_BaudRate = 9600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  /* Configure USART1 */
  USART_Init(USART1, &USART_InitStructure);
  
  /* Enable USART1 Receive and Transmit interrupts */
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

  /* Enable the USART1 */
  USART_Cmd(USART1, ENABLE);   
}

/* RETARGET PRINTF */
int fputc(int ch, FILE *stream) {
  while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
  
  USART_SendData(USART1, (char) ch);

  return ch;
}

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
  
  /* CLCD Configuration */
  CLCD_Configuration();
  
  /* GLCD Configuration */
  GLCD_Configuration();
  
  /* MAL configuration */
  Set_System();
  
  /* UMS configuration */
  Set_USBClock();
  USB_Interrupts_Config();
  USB_Init();
  
  // For TCP client's connection request delay
  presentTime = my_time;
  
  /* RTC configuration by setting the time by Serial USART1 */
  //RTC_SetTimeBySerial();
  
  // When everything is set, print message
  printf("\r\n - System is ready - ");
  
  while (1) {
    if(TimerCount >= 1000) {  // 1000 = 1 sec
      TimerCount = 0;
      
      my_time++;  // uncomment when tcp connection is needed
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
      
      int x, y, z;
      x = mAxisBuf.tmp_data_x_lcd[arrIdx];
      y = mAxisBuf.tmp_data_y_lcd[arrIdx];
      z = mAxisBuf.tmp_data_z_lcd[arrIdx];
      
      char buffer[20];
      sprintf(buffer, "\r\nCountSendByte : %d\n%+d\n%+d\n%+d\n------------------",
              CountSendByte, x, y, z);
      printf(buffer);
      
#elif defined USE_EQDAS_SERVER
      
      char *x, *y, *z;
      x = DAQBoardOne[arrIdx].AxisX;
      y = DAQBoardOne[arrIdx].AxisY;
      z = DAQBoardOne[arrIdx].AxisX;
      
      char buffer[20];
      sprintf(buffer, "X : %s, Y : %s, Z : %s\n", x, y, z);
      //printf("\r\nRX_BUF : %s, strlen(RX_BUF) : %d", (char*)RX_BUF, strlen((char*)RX_BUF));
      printf(buffer);
      
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
      
      /* EQDAQ01, 02 Client Routine ---------------------------------------------*/
      /* E1Flag or E2Flag set when client board successfully connect to server --*/
      /* Refer to wiz820.c line no. 300 for which flag to be set */
      if(E1Flag) {
        int x, y, z;
        x = mAxisBuf.tmp_data_x_lcd[arrIdx];
        y = mAxisBuf.tmp_data_y_lcd[arrIdx];
        z = mAxisBuf.tmp_data_z_lcd[arrIdx];
        
        char E1_Buf[20];
        sprintf(E1_Buf, "%+d,%+d,%+d", x, y, z);
        //sprintf(E1_Buf, "Hello, My Name is Larry G");   // DEBUG
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

