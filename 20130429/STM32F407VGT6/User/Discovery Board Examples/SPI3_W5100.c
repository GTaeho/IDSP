/**
  ******************************************************************************
  * @file    SPI/SPI_TwoBoards/SPI_DataExchangeInterrupt/main.c
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    18-January-2013
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include "stm32f4_discovery.h"

#include "types.h"
#include "w5100.h"
#include "socket.h"

/** @addtogroup STM32F4xx_StdPeriph_Examples
  * @{
  */

/** @addtogroup SPI_DataExchangeInterrupt
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
// MY Mac Address : D2.88.DC.75.3C.8E
#define MY_NET_MAC		{0xD2, 0x08, 0xDC, 0x75, 0x3C, 0x8E}
#define MY_NET_GWIP             {192,  168, 0,    1}	//Gateway     : 192.168.0.1
#define MY_SOURCEIP		{192,  168, 0,   10}	//보드 IP     : 192.168.0.10
#define MY_SUBNET		{255, 255, 255,   0}

#define MY_LISTEN_PORT		8080		//서버 포트 : 8080

#define MAX_BUF_SIZE            12

#define SOCK_TCPS		0
#define MY_NET_MEMALLOC	        0x55	// MY iinchip memory allocation

#define __DEF_IINCHIP_DBG__

/* Parse definition ---- */
enum { False, True };
/* -------------------- */

/* UART4 message buffersize  */
#define RxBufferSize   0x32
/* -------------------------- */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint16_t PrescalerValue = 0;
uint8_t TimerCounter = 0;
uint8_t CheckCounter = 0;
uint8_t getSn = 0;
uint8_t lis = 0;
volatile uint8_t length = 0;

// UART4 Messages buffer
char RxBuffer[RxBufferSize];
volatile unsigned char RxCounter = 0;
volatile unsigned char ParseUART4;

// Systick variable
__IO uint32_t TimingDelay = 0;

// RCC Check variable
volatile uint32_t SYSCLK_Freq;
volatile uint32_t HCLK_Freq;
volatile uint32_t PCLK1_Freq;
volatile uint32_t PCLK2_Freq;

// Send Flag
uint8_t SendFlag = False;

// Temporary order variable
uint16_t order;

// EQ SAMPLE increment
uint16_t inc = 0;

// Dummy EQ Sample
const char *EQ_SAMPLE[] = {
"+0000",  "+0048",  "+4162",
"-0001",  "+0050",  "+4153", 
"-0001",  "+0052",  "+4141", 
"-0005",  "+0054",  "+4151", 
"+0001",  "+0049",  "+4134", 
"+0003",  "+0049",  "+4134", 
"+0005",  "+0050",  "+4142", 
"+0000",  "+0045",  "+4143", 
"-0001",  "+0048",  "+4141", 
"+0010",  "+0047",  "+4133", 
"+0004",  "+0052",  "+4144", 
"+0001",  "+0046",  "+4145", 
"+0004",  "+0056",  "+4139", 
"+0001",  "+0056",  "+4143", 
"-0009",  "+0052",  "+4152", 
"-0005",  "+0053",  "+4137", 
"+0003",  "+0056",  "+4129", 
"+0017",  "+0051",  "+4142", 
"-0001",  "+0050",  "+4154", 
"+0004",  "+0047",  "+4146", 
"+0012",  "+0052",  "+4135", 
"+0001",  "+0057",  "+4141", 
"+0002",  "+0047",  "+4141", 
"-0009",  "+0048",  "+4132", 
"+0005",  "+0044",  "+4146", 
"-0008",  "+0047",  "+4143", 
"+0005",  "+0049",  "+4118", 
"-0003",  "+0056",  "+4167", 
"-0011",  "+0045",  "+4152", 
"+0007",  "+0054",  "+4095", 
"+0007",  "+0051",  "+4145", 
"-0012",  "+0042",  "+4173", 
"-0010",  "+0045",  "+4127", 
"+0005",  "+0060",  "+4097", 
"-0005",  "+0038",  "+4175", 
"+0005",  "+0043",  "+4156", 
"+0006",  "+0055",  "+4110", 
"+0012",  "+0061",  "+4140", 
"+0000",  "+0060",  "+4172", 
"+0004",  "+0049",  "+4140", 
"+0002",  "+0055",  "+4116", 
"+0003",  "+0048",  "+4132", 
"-0002",  "+0047",  "+4155", 
"+0003",  "+0055",  "+4141", 
"-0004",  "+0060",  "+4123", 
"-0005",  "+0054",  "+4159", 
"-0002",  "+0048",  "+4141", 
"-0001",  "+0047",  "+4147", 
"-0005",  "+0049",  "+4146", 
"+0005",  "+0045",  "+4146", 
"+0010",  "+0044",  "+4144", 
"-0004",  "+0047",  "+4143", 
"-0005",  "+0049",  "+4125", 
"+0003",  "+0044",  "+4132", 
"+0005",  "+0041",  "+4146", 
"-0002",  "+0043",  "+4157", 
"+0001",  "+0055",  "+4134", 
"-0003",  "+0057",  "+4137", 
"+0001",  "+0053",  "+4145", 
"-0006",  "+0050",  "+4139", 
"+0001",  "+0047",  "+4135", 
"+0000",  "+0057",  "+4144", 
"-0002",  "+0045",  "+4136", 
"+0009",  "+0041",  "+4127", 
"+0016",  "+0051",  "+4151", 
"-0004",  "+0052",  "+4142", 
"+0005",  "+0052",  "+4143", 
"+0003",  "+0062",  "+4129", 
"-0002",  "+0042",  "+4168", 
"+0005",  "+0044",  "+4135", 
"-0006",  "+0058",  "+4108", 
"-0002",  "+0048",  "+4138", 
"+0006",  "+0039",  "+4143", 
"+0005",  "+0053",  "+4133", 
"+0000",  "+0042",  "+4141", 
"+0008",  "+0049",  "+4148", 
"+0005",  "+0053",  "+4138", 
"+0000",  "+0057",  "+4149", 
"-0002",  "+0054",  "+4131", 
"-0003",  "+0051",  "+4117", 
"+0007",  "+0050",  "+4129", 
"+0004",  "+0041",  "+4141", 
"+0005",  "+0066",  "+4168", 
"-0006",  "+0041",  "+4139", 
"+0001",  "+0058",  "+4118", 
"-0003",  "+0049",  "+4147", 
"+0007",  "+0038",  "+4148", 
"+0011",  "+0049",  "+4135", 
"+0005",  "+0045",  "+4136", 
"+0003",  "+0039",  "+4140", 
"+0007",  "+0049",  "+4131", 
"+0004",  "+0043",  "+4154", 
"-0007",  "+0042",  "+4157", 
"+0002",  "+0045",  "+4151", 
"+0002",  "+0049",  "+4134", 
"-0004",  "+0044",  "+4144", 
"+0001",  "+0047",  "+4155", 
"-0002",  "+0042",  "+4129", 
"+0002",  "+0045",  "+4112", 
"+0010",  "+0050",  "+4154"
};

// Current Order flag
volatile char CurOrder = 0;

/* Private function prototypes -----------------------------------------------*/
void iinchip_init();
void setSHAR(uint8_t * addr);
void setGAR(uint8_t * addr);
void setSUBR(uint8_t * addr);
void setSIPR(uint8_t * addr);
void sysinit(uint8_t tx_size, uint8_t rx_size);
void printSysCfg(void);
void Delay(__IO uint32_t nTime);
void TimingDelay_Decrement(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Configures the different system clocks.
  * @param  None
  * @retval None
  */
void RCC_Config(void) {
  RCC_ClocksTypeDef RCC_ClockFreq;
  
  RCC_GetClocksFreq(&RCC_ClockFreq);
  
  SYSCLK_Freq = RCC_ClockFreq.SYSCLK_Frequency;
  HCLK_Freq = RCC_ClockFreq.HCLK_Frequency;
  PCLK1_Freq = RCC_ClockFreq.PCLK1_Frequency;
  PCLK2_Freq = RCC_ClockFreq.PCLK2_Frequency;
  
  /* GPIOD Periph clock enable */
  /* Green -> PD12, Orange -> PD13, Red -> PD14, Blue -> PD15 */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
  
  /* TIM4 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
  
  /* USART4 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
  
  /* Enable the SPI clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
  
  /* Enable GPIOC for SPI3 clocks */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
  
  /* Enable GPIOA for NSS clocks */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
}

/**
  * @brief  Configures the general purpose input output port.
  * @param  None
  * @retval None
  */
void GPIO_Config(void) {
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Configure LD4, 3, 5, 6 Pins in output mode */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  
  /* configure AF */
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource0, GPIO_AF_UART4);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_UART4);
    
  /* Configure UART4 Tx as alternate function */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* Configure UART4 Rx as input mode */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* Connect SPI pins to AF5 */  
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_SPI3);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_SPI3);    
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_SPI3);

  /* SPI SCK pin configuration */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  /* SPI  MISO pin configuration */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  /* SPI  MOSI pin configuration */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  /* SPI  NSS pin configuration */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void TIM4_Config(void) {
 /* -----------------------------------------------------------------------
    TIM4 Configuration: Output Compare Timing Mode:
    
    In this example TIM4 input clock (TIM4CLK) is set to 2 * APB1 clock (PCLK1), 
    since APB1 prescaler is different from 1.   
      TIM4CLK = 2 * PCLK1
      PCLK1 = HCLK / 4
      => TIM4CLK = HCLK / 2 = SystemCoreClock /2
	  (HCLK = 168MHz, PCLK1 = 42MHz, TIM4LCK = 84MHz)
          
    To get TIM4 counter clock at 84 KHz, the prescaler is computed as follows:
       Prescaler = (TIM4CLK / TIM4 counter clock) - 1
       Prescaler = ((SystemCoreClock /2) /84 KHz) - 1
                                              
    CC1 update rate = TIM4 counter clock / CCR1_Val = 5 Hz
    ==> Toggling frequency = 2.5 Hz

    Note: 
     SystemCoreClock variable holds HCLK frequency and is defined in system_stm32f4xx.c file.
     Each time the core clock (HCLK) changes, user had to call SystemCoreClockUpdate()
     function to update SystemCoreClock variable value. Otherwise, any configuration
     based on this variable will be incorrect.
  ----------------------------------------------------------------------- */  
    
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

  /* Compute the prescaler value */
  PrescalerValue = (uint16_t) ((SystemCoreClock / 2) / 42E6) - 1;

  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 65535;
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

  /* Prescaler configuration */
  TIM_PrescalerConfig(TIM4, PrescalerValue, TIM_PSCReloadMode_Immediate);

  /* TIM Interrupts enable */
  TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);

  /* TIM4 enable counter */
  TIM_Cmd(TIM4, ENABLE);
}

void UART4_Config(void) {
  USART_InitTypeDef USART_InitStructure;
  
  /* UART4 configuration ------------------------------------------------------*/
  /* UART4 configured as follow:
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

  /* Configure UART4 */
  USART_Init(UART4, &USART_InitStructure);
 
  /* Enable the UART4 Receive interrupt: this interrupt is generated when the
     UART4 receive data register is not empty */
  USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);
  
  /* Enable the UART4 */
  USART_Cmd(UART4, ENABLE);
}

/**
  * @brief  Configures the SPI Peripheral.
  * @param  None
  * @retval None
  */
void SPI3_Config(void) {
  SPI_InitTypeDef  SPI_InitStructure;
  
  /* SPI configuration -------------------------------------------------------*/
  SPI_I2S_DeInit(SPI3);
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  
  SPI_Init(SPI3, &SPI_InitStructure);
  
  /* Enable the SPI peripheral */
  SPI_Cmd(SPI3, ENABLE);
}

/**
  * @brief  Configures the nested vectored interrupt controller.
  * @param  None
  * @retval None
  */
void NVIC_Config(void) {
  NVIC_InitTypeDef NVIC_InitStructure;

  /* Enable the TIM4 gloabal Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  /* Enable the UART4 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

void NET_Config(void) {
  unsigned char mac[6]   = MY_NET_MAC;
  unsigned char sm[4]	 = MY_SUBNET;
  unsigned char gwip[4]	 = MY_NET_GWIP;
  unsigned char m_sip[4] = MY_SOURCEIP;
  
  //W5100 Chip Init
  iinchip_init();

  //Set MAC Address
  setSHAR(mac);
  
  //Set Subnet Mask
  setSUBR(sm);

  //Set Gateway
  setGAR(gwip);

  //Set My IP
  setSIPR(m_sip);
  
#ifdef __DEF_IINCHIP_INT__
  //setIMR(0xEF);
#endif

  sysinit(MY_NET_MEMALLOC, MY_NET_MEMALLOC);
  
  printSysCfg();
}

void printSysCfg(void) {
  uint8 tmp_array[6];
  
  printf("\r\n=========================================\r\n");
  printf("W5100-Cortex M3                   \r\n");        
  printf("Network Configuration Information \r\n");        
  printf("=========================================");         		
  
  printf("\r\nMAC : %.2X.%.2X.%.2X.%.2X.%.2X.%.2X", IINCHIP_READ(SHAR0+0),
         IINCHIP_READ(SHAR0+1),IINCHIP_READ(SHAR0+2),IINCHIP_READ(SHAR0+3),
         IINCHIP_READ(SHAR0+4),IINCHIP_READ(SHAR0+5));
  
  getSIPR (tmp_array);
  printf("\r\nIP : %d.%d.%d.%d", tmp_array[0],tmp_array[1],tmp_array[2],tmp_array[3]);
  
  getSUBR(tmp_array);
  printf("\r\nSN : %d.%d.%d.%d", tmp_array[0],tmp_array[1],tmp_array[2],tmp_array[3]);
  
  getGAR(tmp_array);
  printf("\r\nGW : %d.%d.%d.%d", tmp_array[0],tmp_array[1],tmp_array[2],tmp_array[3]);
}

//수신데이터 처리
void EthernetTest(unsigned char *pRcvBuffer, unsigned int len) {
  unsigned int i;
  
  printf("Read Data[%d]\r\n", len);
  
  for(i=0;i<len;i++) {
    //수신데이터 표시
    printf("%c ", pRcvBuffer[i]);
  }
  
  //데이터 처리 - LED제어
  if(pRcvBuffer[0] == '1') {
    GPIO_SetBits(GPIOD, GPIO_Pin_14);
  } else if(pRcvBuffer[0] == '0') {
    GPIO_SetBits(GPIOD, GPIO_Pin_15);
  }
}

//TCP-Server 처리
void ProcessTcpSever(void) {
  // if len is not volatile, it will overflow
  // and causes 'cstack' overstack error
  volatile int len; 
  
  uint8_t *data_buf = (uint8_t) TX_BUF;
  uint16 max_bufsize = sizeof(data_buf);
  
  unsigned int port = MY_LISTEN_PORT;
  
  getSn = getSn_SR(SOCK_TCPS);
  
  switch (getSn_SR(SOCK_TCPS)) {
    case SOCK_INIT:
      lis = listen(SOCK_TCPS);
      break;
      
    case SOCK_ESTABLISHED:
      /* check Rx data */
      if ((len = getSn_RX_RSR(SOCK_TCPS)) > 0) {
        /* if Rx data size is lager than TX_RX_MAX_BUF_SIZE */
        if (len > max_bufsize) len = max_bufsize;
        /* the data size to read is MAX_BUF_SIZE. */
        /* read the received data */
        CheckCounter++;
        len = recv(SOCK_TCPS, data_buf, len);
        length = len;
        
        SendFlag = True;
        
        /* send the received data */
        //send(SOCK_TCPS, data_buf, len);
      }
      break;
      
  case SOCK_CLOSE_WAIT:
      SendFlag = False;
      //If the client request to close
      disconnect(SOCK_TCPS);
      
      break;
      
    case SOCK_CLOSED:      
      //reinitialize the socket
      if(socket(SOCK_TCPS, Sn_MR_TCP, port ,0x00) == 0) {
        printf("Fail to create socket.");
      } else {
        SendFlag = False;
        lis = listen(SOCK_TCPS);
      }
      
      break;
  }
}

/* RETARGET PRINTF */
int fputc(int ch, FILE *stream) {
  while(USART_GetFlagStatus(UART4, USART_FLAG_TXE) == RESET);
  
  USART_SendData(UART4, (char) ch);

  return ch;
}

int convertSample(const char *sample) {
  char result[] = "";
  char *zero = "0";
  char *apd = NULL;
  char i;
  
  strcpy(result, sample);
  for (i = 1; i < 5; i++) {
    apd = strstr(result + i, zero);
    if (result + i == apd) {
      continue;
    } else {
      strcpy(result + 1, result + i);
      break;
    }
  }
  return atoi(result);
}

void print_uvar(const char *sample) {
  char result[] = "";
  char *zero = "0";
  char *apd = NULL;

  strcpy(result, sample);
  apd = strstr(result + 1, zero);
  printf("%s\t", result);
  printf("result+1 = %p,\tapd = %p\t", result + 1, apd);
  apd = strstr(result + 2, zero);
  printf("result + 2 = %p\t,%s\t,apd = %p\t", result + 2, result + 2, apd);
  apd = strstr(result + 3, zero);
  printf("\nresult + 3 = %p\t,%s\t,apd = %p\t", result + 3, result + 3, apd);
  apd = strstr(result + 4, zero);
  printf("result + 4 = %p\t,%s\t,apd = %p\t", result + 4, result + 4, apd);
}

/**
  * @brief  This function handles TIM4 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM4_IRQHandler(void) {
  TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
    
  TimerCounter++;
}

/**1
  * @brief  This function handles UART4 global interrupt request.
  * @param  None
  * @retval None
  */
void UART4_IRQHandler(void) {
  if(USART_GetITStatus(UART4, USART_IT_RXNE) != RESET) {
    RxBuffer[RxCounter] = USART_ReceiveData(UART4) & 0xFF;
    if(RxBuffer[RxCounter] == '\n') {
      while(RxBuffer[RxCounter + 1] != 0) {
          RxBuffer[RxCounter + 1] = 0;
      }
      ParseUART4 = True;
      RxCounter = 0;
    } else {
      RxCounter++;
    }
  }
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void) {
  TimingDelay_Decrement();
}

/**
  * @brief  Inserts a delay time.
  * @param  nTime: specifies the delay time length, in milliseconds.
  * @retval None
  */
void Delay(__IO uint32_t nTime) {
  TimingDelay = nTime;

  while(TimingDelay != 0);
}

/**
  * @brief  Decrements the TimingDelay variable.
  * @param  None
  * @retval None
  */
void TimingDelay_Decrement(void) {
  if (TimingDelay != 0x00) { 
    TimingDelay--;
  }
}

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void) {
  /*!< At this stage the microcontroller clock setting is already configured, 
       this is done through SystemInit() function which is called from startup
       files (startup_stm32f40xx.s/startup_stm32f427x.s) before to branch to 
       application main. 
       To reconfigure the default setting of SystemInit() function, refer to
       system_stm32f4xx.c file
     */
  
  /* RCC configuration */
  RCC_Config();
  
  /* GPIO configuration */
  GPIO_Config();
  
  /* TIM4 configuration */
  TIM4_Config();
  
  /* UART4 configuration */
  UART4_Config();
  
  /* SPI3 configuration */
  SPI3_Config();
  
  /* NVIC configuration */
  NVIC_Config();
  
  /* Network Configuration */
  NET_Config();
  
  /* Configure SysTick */  
  SysTick_Config(SystemCoreClock / 1000);

  /* Infinite Loop */
  while (1) {
    if(TimerCounter >= 10) {
      TimerCounter = 0;
      
      if(SendFlag) {
        //SendFlag = False;
        
        char str[30];
        if(order++ < 90) {
          float one = NULL;
          float two = NULL;
          float thr = NULL;
            
          if(inc <= 300) {
            one = convertSample(EQ_SAMPLE[inc++]) * 1e-7;
            two = convertSample(EQ_SAMPLE[inc++]) * 1e-7;
            thr = convertSample(EQ_SAMPLE[inc++]) * 1e-7;  
          } else {
            inc = 0;            
          }
          
          sprintf(str, "%-0.7f,%-0.7f,%-0.7f\n", one, two, thr);
        } else {
          order = 0;
        }
        
        // Only when socket is established, send data
        if(getSn_SR(SOCK_TCPS) == SOCK_ESTABLISHED) {
          /* send the received data */
          send(SOCK_TCPS, (uint8*)str, strlen(str));
        }
      }
    }
    
    if(ParseUART4) {
      ParseUART4 = False;
      
      printSysCfg();
    }
    
    ProcessTcpSever();
  }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name3
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {}
}
#endif

/**
  * @}
  */

/**
  * @}
  */

/******************END OF FILE******************/
