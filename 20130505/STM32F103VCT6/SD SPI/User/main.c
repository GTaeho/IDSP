/* Includes ------------------------------------------------------------------*/
#include "ff.h"
#include "stm32f10x.h"
#include "SPI_MSD_Driver.h"
#include <stdio.h>
#include <string.h>

/* Private define ------------------------------------------------------------*/
/* -- U1Tx = PA9(68¹øÇÉ), U1Rx = PA10(69¹øÇÉ)-- */
#define USART1_Pin_Tx     GPIO_Pin_9
#define USART1_Pin_Rx     GPIO_Pin_10
/* -------------------------------------------- */

/* Private variables ---------------------------------------------------------*/
FATFS fs;         /* Work area (file system object) for logical drive */
FIL fsrc;         /* file objects */
FRESULT res;
UINT br;

char path[512]="0:";
uint8_t textFileBuffer[] = "Sample Text of DEMO.txt\r\n";   

/* Private function prototypes -----------------------------------------------*/
int SD_TotalSize(void);
void USART_Configuration(void);
FRESULT scan_files (char* path);

void  Delay (uint32_t nCount) {
  for(; nCount != 0; nCount--);
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

void USART_Configuration(void) {
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure; 

  RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO ,ENABLE);

  /* Configure USART1 Tx as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = USART1_Pin_Tx;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* Configure USART1 Rx as input floating */
  GPIO_InitStructure.GPIO_Pin = USART1_Pin_Rx;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* USART1 configuration ------------------------------------------------------*/
  /* USART1 configured as follow:
        - BaudRate = 9600 baud  
        - Word Length = 8 Bits
        - One Stop Bit
        - No parity
        - Hardware flow control disabled (RTS and CTS signals)
        - Receive and transmit enabled
  */
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

int SD_TotalSize(void) {
  FATFS *fs;
  DWORD fre_clust;        
  
  res = f_getfree("0:", &fre_clust, &fs);
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

/* RETARGET PRINTF */
int fputc(int ch, FILE *stream) {
  while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
  
  USART_SendData(USART1, (char) ch);

  return ch;
}

/*******************************************************************************
* Function Name  : main
* Description    : Main program
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
int main(void) {
  USART_Configuration();
  MSD_SPI_Configuration();

  f_mount(0, &fs);

  res = f_open( &fsrc, "0:/Demo.TXT", FA_CREATE_NEW | FA_WRITE);

  if ( res == FR_OK ) { 
    /* Write buffer to file */
    res = f_write(&fsrc, textFileBuffer, sizeof(textFileBuffer), &br);     

    printf("Demo.TXT successfully created\r\n");
  
    /*close file */
    f_close(&fsrc);
  } else if ( res == FR_EXIST ) {
    printf("Demo.TXT created in the disk\r\n");
  }

  scan_files(path);
  SD_TotalSize();

  /* Infinite loop */
  while (1);
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
