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
#include "calendar.h"
#include "rtc.h"
#include "fatfs.h"
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* FatFs elements from config.c */
extern FATFS fs;         /* Work area (file system object) for logical drive */
extern FIL fsrc;         /* file objects */
extern FRESULT res;
extern UINT br;

// FatFs path
extern char path[512];

// Time criteria
uint32_t my_time;
uint32_t presentTime;

// Array index order
int E1Order = 0, E2Order = 0;;

// for faster, efficient performance variable
int arrIdx = 0;

// TimerCount from stm32f10x_it.c
extern volatile uint16_t TimerCount;
extern volatile uint16_t ClientTimerCounter;
extern volatile int TenMilliSecCount;
extern volatile int RTCTIM6Count;

// UART1 Messages buffer from stm32f10x_it.c
extern char RxBuffer[RxBufferSize];
extern volatile unsigned char RxCounter;
extern volatile unsigned char ParseUSART1;

// GLCD Graph container from stm32f10x_it.c
extern unsigned char mode, i, j, x, y, offset_start, offset_finish;
extern volatile int index;
extern volatile int flag_uart;

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

// export buffer to send to PC
extern EQ_DAQ_ONE DAQBoardOne[100];

// RTC 1 second flag from stm32f10x_it.
extern __IO uint32_t RTCTimeDisplay;

// RTC Alarm Flag
extern volatile int RTCAlarmFlag;

// Time variable updates every second from rtc.c
extern __IO uint32_t THH, TMM, TSS;

// go append data flag, each second flag from config.c
extern bool GoAppendDataFlag, EachSecFlag, FileRecordCompleteFlag;

/* Text Parser delimiter */
enum { EQ_ONE, EQ_TWO, PC_Cli, PC_DUMP };

// Calendar Data
volatile int year, month, day, hour, minute, second, tmsecond;

// Directory, File string container pointer
char *filePath;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

