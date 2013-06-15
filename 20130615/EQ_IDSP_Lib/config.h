#ifndef _CONFIG_H_
#define _CONFIG_H_

/* Includes ------------------------------------------------------------------*/
#include "types.h"
#include "calendar.h"
#include <stdlib.h>

/* Function Setup ------------------------------------------------------------*/
/* Uncomment following line to choose a board you wish to use */
 #define USE_EQDAS01 //Earthquake DataAquisition System 01
// #define USE_EQDAS02 //Earthquake DataAquisition System 02
// #define USE_EQDAS_SERVER //Earthquake DataAquisition System Main Board as SERVER 

/* Private typedef -----------------------------------------------------------*/
typedef struct _CONFIG_MSG {
  uint8 Mac[6];
  uint8 Lip[4];
  uint8 Sub[4];
  uint8 Gw[4];
  uint8 DNS_Server_IP[4];	
  uint8  DHCP;
}CONFIG_MSG;


typedef struct _CONFIG_TYPE_DEF {
  uint16 port;
  uint8 destip[4];
}CHCONFIG_TYPE_DEF;

// struct variables to be send to PC Client
typedef struct {
  char *Date;
  char *Time;
  char *AxisX;
  char *AxisY;
  char *AxisZ;
}EQ_DAQ_ONE;

typedef struct {
  char *Date;
  char *Time;
  char *AxisX;
  char *AxisY;
  char *AxisZ;
}EQ_DAQ_TWO;


/* Private define ------------------------------------------------------------*/
#define SOCK_CONFIG		2	// UDP
#define SOCK_DNS		2	// UDP
#define SOCK_DHCP		3	// UDP

#define MAX_BUF_SIZE		1460
#define KEEP_ALIVE_TIME	30	// 30sec

#define ON	1
#define OFF	0

#define HIGH	1
#define LOW		0

#define __GNUC__

// SRAM address range is 0x2000 0000 ~ 0x2000 4FFF (20KB)
#define TX_RX_MAX_BUF_SIZE	2048
//#define TX_BUF	0x20004000
//#define RX_BUF	(TX_BUF+TX_RX_MAX_BUF_SIZE)
extern uint8 TX_BUF[TX_RX_MAX_BUF_SIZE];
extern uint8 RX_BUF[TX_RX_MAX_BUF_SIZE];

// Time variable updates every second from rtc.c
extern __IO uint32_t THH, TMM, TSS;

// EQDAS Configuration IP, PORT
extern __IO uint8 IP[4];
extern __IO uint16 EQDAS_Conf_PORT;

// ATFC Algorithm Parameters
extern volatile int Alpha;
extern volatile int Beta;
extern volatile float Gamma;
extern volatile int L;
extern volatile int T;
extern volatile int RefreshTime;
extern volatile int M;
extern volatile int N;


#define ApplicationAddress 	0x08004000

/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
void RCC_Configuration(void);
void TIM2_Configuration(void);
void TIM3_Configuration(void);
void TIM4_Configuration(void);
void TIM6_Configuration(void);
void UART_Configuration(void);
void ATFC_GPIO_Configuration(void);
void CopyToTmpArray(uint8_t index);
void CopyToTmpGalArray(uint8_t index);
void CutOffTo1G(uint8_t index);
void CalculateGalAndCopyToGal(uint8_t index);
void DetermineKMA(uint8_t index);
void ProcessParameterStream(void);
void ProcessTextStream(uint8_t which, char *string, int index);
void SendToPC(int index);
void CheckSignAndToInt(int index);
void CopyToFatFsDataBuffer(int index);

#endif

