/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f10x_it.h"
#include "config.h"
#include "usb_type.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Text Parser delimiter */
typedef enum { EQ_ONE, EQ_TWO, PC_Cli }BDELIMITER;

/* Private variables ---------------------------------------------------------*/

/* FatFs elements */
FATFS fs;         /* Work area (file system object) for logical drive */
FIL fsrc;         /* file objects */
FRESULT res;
UINT br;

// FatFs path
char path[512]="0:";

// RCC Check variable
volatile uint32_t SYSCLK_Freq;
volatile uint32_t HCLK_Freq;
volatile uint32_t PCLK1_Freq;
volatile uint32_t PCLK2_Freq;

// AxisData Buffer typedef struct from stm32f4xx_it.h
extern AXISDATA mAxisData;
extern AXISBUF  mAxisBuf;

// GAL container
double tmp_gal = 0;
unsigned short max_gal = 0;

// GAL Buffer originally called here
GALBUF   mGalBuf;

// 10 seconds enough data to retain which from config.h
EQ_DAQ_ONE DAQBoardOne[100];
EQ_DAQ_ONE DAQBoardTwo[100];

// String Container from main.c
extern char DATA1_BUF[19500];
extern char DATA2_BUF[19500];

// Each second count
int OneSecCount = 0;

// Every 5 second flag
bool FiveSecFlag = false;

// Go ahead and append to the file flag
bool GoAppendDataFlag = false;

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

/**
  * @brief  Configures the USART1 periphral.
  * @param  None
  * @retval None
  */
void UART_Configuration(void) {
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

FRESULT scan_files (char* path) {
  FILINFO fno;
  DIR dir;
  int i;
  char *fn;
#if _USE_LFN
  static char lfn[_MAX_LFN * (_DF1S ? 2 : 1) + 1];
  fno.lfname = lfn;
  fno.lfsize = sizeof(lfn);
#endif
  res = f_opendir(&dir, path);
  if (res == FR_OK) {
    i = strlen(path);
    for (;;) {
      res = f_readdir(&dir, &fno);
      if (res != FR_OK || fno.fname[0] == 0) break;
      if (fno.fname[0] == '.') continue;
#if _USE_LFN
      fn = *fno.lfname ? fno.lfname : fno.fname;
#else
      fn = fno.fname;
#endif
      if (fno.fattrib & AM_DIR) {
          sprintf(&path[i], "/%s", fn);
          res = scan_files(path);
          if (res != FR_OK) break;
          path[i] = 0;
      } else {
          printf("%s/%s \r\n", path, fn);
      }
    }
  }

  return res;
}


int SD_TotalSize(void) {
  FATFS *fs;
  DWORD fre_clust;        
  
  res = f_getfree("0:", &fre_clust, &fs);
  printf("\r\n--------------------------");
  printf("\r\n * Total space in SD Card");
  if ( res==FR_OK ) {
    /* Print free space in unit of MB (assuming 512 bytes/sector) */
    printf("\r\n%d MB total drive space.\r\n"
         "%d MB available.\r\n",
         ( (fs->n_fatent - 2) * fs->csize ) / 2 /1024 , (fre_clust * fs->csize) / 2 /1024 );
              
    return ENABLE;
  } else {
    return DISABLE; 
  }
}

void FPrintFatResult(FRESULT tResult) {
  switch(tResult) {
    case FR_OK:
      printf("FR_OK\n");
      break;
    case FR_NOT_READY:
      printf("FR_NOT_READY\n");
      break;
    case FR_NO_FILE:
      printf("FR_NO_FILE\n");
      break;
    case FR_NO_PATH:
      printf("FR_NO_PATH\n");
      break;
    case FR_INVALID_NAME:
      printf("FR_INVALID_NAME\n");
      break;
    case FR_INVALID_DRIVE:
      printf("FR_INVALID_DRIVE\n");
      break;
    case FR_DENIED:
      printf("FR_DENIED\n");
      break;
    case FR_EXIST:
      printf("FR_EXIST\n");
      break;
    case FR_WRITE_PROTECTED:
      printf("FR_WRITE_PROTECTED\n");
      break;
    case FR_NOT_ENABLED:
      printf("FR_NOT_ENABLED\n");
      break;
    case FR_NO_FILESYSTEM:
      printf("FR_NO_FILESYSTEM\n");
      break;
    case FR_INVALID_OBJECT:
      printf("FR_INVALID_OBJECT\n");
      break;
    case FR_MKFS_ABORTED:
      printf("FR_MKFS_ABORTED\n");
      break;
  }
}

// Copy untrimmed data to GAL array with its crude data trimmed
void CopyToTmpGalArray(uint8_t index) {
  // for code efficiency
  uint8_t arrIdx = index;
  
  if(mAxisBuf.tmp_data_x_lcd[arrIdx] & 0x2000)
    mGalBuf.tmp_gal_x[arrIdx] = (~mAxisBuf.tmp_data_x_lcd[arrIdx] + 1) & 0x1FFF;
  else
    mGalBuf.tmp_gal_x[arrIdx] = mAxisBuf.tmp_data_x_lcd[arrIdx] & 0x1FFF;
    
  if(mAxisBuf.tmp_data_y_lcd[arrIdx] & 0x2000)
    mGalBuf.tmp_gal_y[arrIdx] = (~mAxisBuf.tmp_data_y_lcd[arrIdx] + 1) & 0x1FFF;
  else
    mGalBuf.tmp_gal_y[arrIdx] = mAxisBuf.tmp_data_y_lcd[arrIdx] & 0x1FFF;
  
  if(mAxisBuf.tmp_data_z_lcd[arrIdx] & 0x2000)
    mGalBuf.tmp_gal_z[arrIdx] = (~mAxisBuf.tmp_data_z_lcd[arrIdx] + 1) & 0x1FFF;
  else
    mGalBuf.tmp_gal_z[arrIdx] = mAxisBuf.tmp_data_z_lcd[arrIdx] & 0x1FFF;
}

// According to theory, Maximum G covers 2Gs. so we need to cut off surplus 1G
// there exist in both +, - area.
void CutOffTo1G(uint8_t index) {
  // for code efficiency
  uint8_t arrIdx = index;
  
  /* Axis X --------------------------------------*/
  //when it is positive value
  if(!(mAxisBuf.tmp_data_x_lcd[arrIdx] & 0x2000)) {
    if(mAxisBuf.tmp_data_x_lcd[arrIdx] & 0x1000) {
      mAxisBuf.tmp_data_x_lcd[arrIdx] = 0x0FFF;
    }
  } else { //when it is negative value
    if(!(mAxisBuf.tmp_data_x_lcd[arrIdx] & 0x1000)) {
      mAxisBuf.tmp_data_x_lcd[arrIdx] = 0x3001;
    }
  }
  
  /* Axis Y --------------------------------------*/
  if(!(mAxisBuf.tmp_data_y_lcd[arrIdx] & 0x2000)) {
    if(mAxisBuf.tmp_data_y_lcd[arrIdx] & 0x1000) {
      mAxisBuf.tmp_data_y_lcd[arrIdx] = 0x0FFF;
    }
  } else {
    if(!(mAxisBuf.tmp_data_y_lcd[arrIdx] & 0x1000)) {
      mAxisBuf.tmp_data_y_lcd[arrIdx] = 0x3001;
    }
  }
  
  /* Axis Z --------------------------------------*/
  if(!(mAxisBuf.tmp_data_z_lcd[arrIdx] & 0x2000)) {
    if(mAxisBuf.tmp_data_z_lcd[arrIdx] & 0x1000) {
      mAxisBuf.tmp_data_z_lcd[arrIdx] = 0x0FFF;
    }
  } else {
    if(!(mAxisBuf.tmp_data_z_lcd[arrIdx] & 0x1000)) {
      mAxisBuf.tmp_data_z_lcd[arrIdx] = 0x3001;
    }
  }  
}

void CalculateGalAndCopyToGal(uint8_t index) {
  // Calculate GAL
  tmp_gal = sqrt(((mGalBuf.tmp_gal_x[index]/4) * (mGalBuf.tmp_gal_x[index]/4)) + 
                 ((mGalBuf.tmp_gal_y[index]/4) * (mGalBuf.tmp_gal_y[index]/4)) + 
                 ((mGalBuf.tmp_gal_z[index]/4) * (mGalBuf.tmp_gal_z[index]/4)));
  
  // Copy to GAL Array
  mAxisData.data_g[index] = (int)tmp_gal;
}

// Determine KMA scale
void DetermineKMA(uint8_t index) {
  if(mAxisData.data_g[index+1] > mAxisData.data_g[index]) {
    max_gal = mAxisData.data_g[index+1];
  } else {
    max_gal = max_gal;
  }
}

void ProcessTextStream(uint8_t which, char *string, int index) {
  char *TokenOne = NULL;
  char *TokenTwo = NULL;
  int arrIdx = index;
  
  switch(which) {
  case EQ_ONE :
    //TokenOne = strtok(string, ","); // result contains Date. ex) 20130428
    //DAQBoardOne[arrIdx].Date = TokenOne;
    //TokenOne = strtok(NULL, ","); // result contains Time. ex) 21384729
    //DAQBoardOne[arrIdx].Time = TokenOne;
    TokenOne = strtok(string, ","); // result contains Axis X Data. ex) +4096
    DAQBoardOne[arrIdx].AxisX = TokenOne;
    TokenOne = strtok(NULL, ","); // result contains Axis Y Data. ex) +4096
    DAQBoardOne[arrIdx].AxisY = TokenOne;
    TokenOne = strtok(NULL, ","); // result contains Axis Z Data. ex) +4096
    DAQBoardOne[arrIdx].AxisZ = TokenOne;
    break;
    
  case EQ_TWO :
    //TokenTwo = strtok(string, ","); // result contains Date. ex) 20130428
    //DAQBoardTwo[arrIdx].Date = TokenTwo;
    //TokenTwo = strtok(NULL, ","); // result contains Time. ex) 21384729
    //DAQBoardTwo[arrIdx].Time = TokenTwo;
    TokenTwo = strtok(string, ","); // result contains Axis X Data. ex) +4096
    DAQBoardTwo[arrIdx].AxisX = TokenTwo;
    TokenTwo = strtok(NULL, ","); // result contains Axis Y Data. ex) +4096
    DAQBoardTwo[arrIdx].AxisY = TokenTwo;
    TokenTwo = strtok(NULL, ","); // result contains Axis Z Data. ex) +4096
    DAQBoardTwo[arrIdx].AxisZ = TokenTwo;
    break;
  case PC_Cli: 
    break;
  default: break;
  }
}

void CheckSignAndToInt(int index) {
  // for code efficiency
  uint8_t arrIdx = index;
  
  /* Axis X --------------------------------------*/
  //when it is positive value
  if(!(mAxisBuf.tmp_data_x_lcd[arrIdx] & 0x2000)) {
    // no extra work needed when it is positive
  } else { //when it is negative value
    mAxisBuf.tmp_data_x_lcd[arrIdx] = (~mAxisBuf.tmp_data_x_lcd[arrIdx] + 1) & 0xFFF;
    mAxisBuf.tmp_data_x_lcd[arrIdx] |= 0xFFFFF000;  // set MSB of int type
  }
  
  /* Axis Y --------------------------------------*/
  if(!(mAxisBuf.tmp_data_y_lcd[arrIdx] & 0x2000)) {
    // no extra work needed when it is positive
  } else {
    mAxisBuf.tmp_data_y_lcd[arrIdx] = (~mAxisBuf.tmp_data_y_lcd[arrIdx] + 1) & 0xFFF;
    mAxisBuf.tmp_data_y_lcd[arrIdx] |= 0xFFFFF000;  // set MSB of int type
  }
  
  /* Axis Z --------------------------------------*/
  if(!(mAxisBuf.tmp_data_z_lcd[arrIdx] & 0x2000)) {
    // no extra work needed when it is positive
  } else {
    mAxisBuf.tmp_data_z_lcd[arrIdx] = (~mAxisBuf.tmp_data_z_lcd[arrIdx] + 1) & 0xFFF;
    mAxisBuf.tmp_data_z_lcd[arrIdx] |= 0xFFFFF000;  // set MSB of int type
  }   
}

/** @Function explanation
  * Copy the processed each axis data to selective data buffer
  * which is big enough to contain 5 second of data and
  * is to be switched on every 5 second to properly propagate
  * massive string data coming from the FPGA.
  */
void CopyToFatFsDataBuffer(int index) {
  int arrIdx = index;
  char strBuf[37];
  
  sprintf(strBuf, "20130518_01340000_%+05d_%+05d_%+05d\r\n", 
          mAxisBuf.tmp_data_x_lcd[arrIdx],
          mAxisBuf.tmp_data_y_lcd[arrIdx],
          mAxisBuf.tmp_data_z_lcd[arrIdx]);
  
  // Count each second to determine FiveSecFlags.
  if(arrIdx == 99) {
    OneSecCount++;
    // When OneSecCount reach to 5, set FiveSecFlag.
    // And when it reach to 10, reset FiveSecFlag and OneSecCount to recount from 0.
    if(OneSecCount == 5) {
      printf("\r\nOneSecCount == 5");
      printf("\r\nstrlen(DATA1_BUF) : %d\r\nstrlen(DATA2_BUF) : %d", 
             strlen(DATA1_BUF), strlen(DATA2_BUF));
      FiveSecFlag = true;
      GoAppendDataFlag = true;  // go ahead and save it!
    } else if(OneSecCount == 10) {
      printf("\r\nOneSecCount == 10");
      printf("\r\nstrlen(DATA1_BUF) : %d\r\nstrlen(DATA2_BUF) : %d", 
             strlen(DATA1_BUF), strlen(DATA2_BUF));
      OneSecCount = 0;
      FiveSecFlag = false;
      GoAppendDataFlag = true;  // set here, have reset in while routine
    }
  }
  
  // following routine must be excuted every single cycle
  if(!FiveSecFlag) {  // from 0 sec to right before 5 sec.
    strcat(DATA1_BUF, strBuf);
  } else {  // remaining 5 sec to end of 9 sec.
    strcat(DATA2_BUF, strBuf);
  }
}

/*------------------------------------------------------------/
/ Open or create a file in append mode
/------------------------------------------------------------*/
FRESULT open_append (
    FIL* fp,            /* [OUT] File object to create */
    const char* path    /* [IN]  File name to be opened */
)
{
    FRESULT fr;

    /* Opens an existing file. If not exist, creates a new file. */
    fr = f_open(fp, path, FA_WRITE | FA_OPEN_ALWAYS);
    if (fr == FR_OK) {
        /* Seek to end of the file to append data */
        fr = f_lseek(fp, f_size(fp));
        if (fr != FR_OK)
            f_close(fp);
    }
    return fr;
}

