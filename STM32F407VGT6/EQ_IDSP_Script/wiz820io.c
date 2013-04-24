/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "config.h"
#include "glcd.h"
#include "w5200.h"
#include "socket.h"
#include <stdio.h>

/* Private macro -------------------------------------------------------------*/
/* Parse definition ---- */
enum { False, True };
/* --------------------- */

/* tick -----------------*/
#define tick_second 1
/* ----------------------*/

/* Private variables ---------------------------------------------------------*/
// Debug
uint8_t getSn = 0;
uint8_t lis = 0;

// Send Flag
uint8_t SendFlag = False;

// typedef initialize
CONFIG_MSG Config_Msg;
CHCONFIG_TYPE_DEF Chconfig_Type_Def; 

// Configuration Network Information of W5200
uint8 Enable_DHCP = OFF;
uint8 MAC[6] = {0xEC, 0x08, 0xDC, 0x01, 0x02, 0x03};  //MAC Address
uint8 IP[4] = {192, 168, 0, 10};                      //IP Address
uint8 GateWay[4] = {192, 168, 0, 1};                  //Gateway Address
uint8 SubNet[4] = {255, 255, 255, 0};                 //SubnetMask Address

//TX MEM SIZE- SOCKET 0:8KB, SOCKET 1:2KB, SOCKET2-7:1KB
//RX MEM SIZE- SOCKET 0:8KB, SOCKET 1:2KB, SOCKET2-7:1KB
uint8 txsize[MAX_SOCK_NUM] = {2,2,2,2,2,2,2,2};
uint8 rxsize[MAX_SOCK_NUM] = {2,2,2,2,2,2,2,2};

//FOR TCP Client
//Configuration Network Information of TEST PC
uint8 Dest_IP[4] = {192, 168, 0, 16}; //DST_IP Address 
uint16 Dest_PORT = 5050; //DST_IP port

uint8 ch_status[MAX_SOCK_NUM] = { 0, };	/** 0:close, 1:ready, 2:connected */

uint8 TX_BUF[TX_RX_MAX_BUF_SIZE]; // TX Buffer for applications
uint8 RX_BUF[TX_RX_MAX_BUF_SIZE]; // RX Buffer for applications

// Socket variable
volatile uint16_t RSR_len;

// Time criteria
extern uint32_t my_time;
extern uint32_t presentTime;

/* Private functions ---------------------------------------------------------*/

void WIZ820io_SPI2_Configuration(void) { // one that is on left one
  GPIO_InitTypeDef GPIO_InitStructure;
  SPI_InitTypeDef  SPI_InitStructure;

  /* RCC configuration -------------------------------------------------------*/  
  /* Enable GPIOB, C, E for SPI2 & RESET pin */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC | 
                         RCC_AHB1Periph_GPIOE, ENABLE);
  
  /* Enable the SPI2 clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
  
  /* GPIO configuration ------------------------------------------------------*/
  /* Connect SPI pins to AFIO */
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_SPI2);  // SPI2_SCK
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource2, GPIO_AF_SPI2);   // SPI2_MISO
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource3, GPIO_AF_SPI2);   // SPI2_MOSI

  /* SPI SCK pin configuration */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  /* SPI  MISO pin configuration */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  /* SPI  MOSI pin configuration */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  /* SPI  NSS pin configuration */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  /* RESET pin configuration */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOE, &GPIO_InitStructure);
  
  /* SPI configuration -------------------------------------------------------*/
  SPI_I2S_DeInit(SPI2);
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  SPI_Init(SPI2, &SPI_InitStructure);
  
  /* Enable the SPI peripheral */
  SPI_Cmd(SPI2, ENABLE);
  
  /* RESET Device ------------------------------------------------------------*/
  GPIO_ResetBits(GPIOE, GPIO_Pin_2);
  Delay_us(2);
  GPIO_SetBits(GPIOE, GPIO_Pin_2);
  Delay_ms(150);
}

void WIZ820io_SPI3_Configuration(void) { // one that is on right one
  GPIO_InitTypeDef GPIO_InitStructure;
  SPI_InitTypeDef  SPI_InitStructure;
  
  /* RCC configuration -------------------------------------------------------*/
  /* Enable GPIOA, C, E for SPI3 & RESET pin */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOC |
                         RCC_AHB1Periph_GPIOE, ENABLE);
  
  /* Enable the SPI3 clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
  
  /* GPIO configuration ------------------------------------------------------*/
  /* Connect SPI pins to AFIO */
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_SPI2);  // SPI3_SCK
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_SPI2);   // SPI3_MISO
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_SPI2);   // SPI3_MOSI

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
  
  /* RESET pin configuration */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOE, &GPIO_InitStructure);
  
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
  
  /* RESET Device ------------------------------------------------------------*/
  GPIO_ResetBits(GPIOE, GPIO_Pin_3);
  Delay_us(2);
  GPIO_SetBits(GPIOE, GPIO_Pin_3);
  Delay_ms(150);
}

void Set_network(void) {
  uint8 i;
  
  // MAC ADDRESS
  for (i = 0 ; i < 6; i++) Config_Msg.Mac[i] = MAC[i];
  
  // Local IP ADDRESS
  Config_Msg.Lip[0] = IP[0]; Config_Msg.Lip[1] = IP[1]; 
  Config_Msg.Lip[2] = IP[2]; Config_Msg.Lip[3] = IP[3];
  
  // GateWay ADDRESS
  Config_Msg.Gw[0] = GateWay[0]; Config_Msg.Gw[1] = GateWay[1]; 
  Config_Msg.Gw[2] = GateWay[2]; Config_Msg.Gw[3] = GateWay[3];
  
  // Subnet Mask ADDRESS
  Config_Msg.Sub[0] = SubNet[0]; Config_Msg.Sub[1] = SubNet[1]; 
  Config_Msg.Sub[2] = SubNet[2]; Config_Msg.Sub[3] = SubNet[3];
  
  setSHAR(Config_Msg.Mac);
  saveSUBR(Config_Msg.Sub);
  setSUBR();
  setGAR(Config_Msg.Gw);
  setSIPR(Config_Msg.Lip);

  // Set DHCP
  Config_Msg.DHCP = Enable_DHCP;    
  //Destination IP address for TCP Client
  Chconfig_Type_Def.destip[0] = Dest_IP[0]; Chconfig_Type_Def.destip[1] = Dest_IP[1];
  Chconfig_Type_Def.destip[2] = Dest_IP[2]; Chconfig_Type_Def.destip[3] = Dest_IP[3];
  Chconfig_Type_Def.port = Dest_PORT;

  //Set PTR and RCR register	
  setRTR(6000);
  setRCR(3);

  //Init. TX & RX Memory size
  sysinit(txsize, rxsize);
}

void printSysCfg(void) {
  uint8 tmp_array[6];
  
  printf("\r\n----------------------------------------- \r\n");
  printf("W5200-Cortex M3                       \r\n");        
  printf("Network Configuration Information \r\n");        
  printf("----------------------------------------- ");         		
  
  printf("\r\nMAC : %.2X.%.2X.%.2X.%.2X.%.2X.%.2X", IINCHIP_READ(SHAR0+0),IINCHIP_READ(SHAR0+1),IINCHIP_READ(SHAR0+2),
                                                                                                                                                             IINCHIP_READ(SHAR0+3),IINCHIP_READ(SHAR0+4),IINCHIP_READ(SHAR0+5));
  
  getSIPR (tmp_array);
  printf("\r\nIP : %d.%d.%d.%d", tmp_array[0],tmp_array[1],tmp_array[2],tmp_array[3]);
  
  getSUBR(tmp_array);
  printf("\r\nSN : %d.%d.%d.%d", tmp_array[0],tmp_array[1],tmp_array[2],tmp_array[3]);
  
  getGAR(tmp_array);
  printf("\r\nGW : %d.%d.%d.%d", tmp_array[0],tmp_array[1],tmp_array[2],tmp_array[3]);
}

uint32_t time_return(void) {
  return my_time;
}

/* ------------ TCP Client Routine ------------ */
void ProcessTcpClient(SOCKET mSocket) {
  // if len is not volatile, it will overflow
  // and causes 'cstack' overstack error
  volatile uint16_t len;
  
  getSn = getSn_SR(mSocket);
  
  switch (getSn_SR(mSocket)) {  
    case SOCK_ESTABLISHED:
      if(ch_status[mSocket]==1) {
        printf("\r\n%d : Connected", mSocket);
        ch_status[mSocket] = 2;
      }
      
      /* check Rx data */
      if ((len = getSn_RX_RSR(mSocket)) > 0) {
        /* if Rx data size is lager than TX_RX_MAX_BUF_SIZE */
        /* the data size to read is MAX_BUF_SIZE. */
        if (len > TX_RX_MAX_BUF_SIZE) len = TX_RX_MAX_BUF_SIZE;
          /* read the received data */
          len = recv(mSocket, RX_BUF, len);
          RSR_len = len;
          
          // send data
          SendFlag = True;
          
          /* sent the received data back */
          //send(mSocket, RX_BUF, len, (bool)WINDOWFULL_FLAG_OFF);
      }
      break;
      
  case SOCK_CLOSE_WAIT: /* If the client request to close */
      SendFlag = False;
      printf("\r\n%d : CLOSE_WAIT", mSocket);

      /* check Rx data */
      if ((len = getSn_RX_RSR(mSocket)) > 0) {
        /* if Rx data size is lager than TX_RX_MAX_BUF_SIZE */
        /* the data size to read is MAX_BUF_SIZE. */
        if (len > TX_RX_MAX_BUF_SIZE) len = TX_RX_MAX_BUF_SIZE;
        /* read the received data */
        len = recv(mSocket, RX_BUF, len);
        RSR_len = len;
      }
      disconnect(mSocket);
      ch_status[mSocket] = 0;
      break;
    
  case SOCK_INIT: /* if a socket is initiated */
      /* For TCP client's connection request delay : 3 sec */
      if(time_return() - presentTime >= (tick_second * 3)) {
        /* Try to connect to TCP server(Socket, DestIP, DestPort) */
        printf("\r\n[ Attempt to connect on socket %d ]", mSocket);
        connect(mSocket, Chconfig_Type_Def.destip, Chconfig_Type_Def.port);
        presentTime = time_return();
      }
      break;
      
    case SOCK_CLOSED: /* if a socket is closed */
      if(!ch_status[mSocket]) {
        printf("\r\n%d : Loop-Back TCP Client Started. port: %d", mSocket, Chconfig_Type_Def.port);
        ch_status[mSocket] = 1;
      }
      
      /* reinitialize the socket */
      if(socket(mSocket, Sn_MR_TCP, Chconfig_Type_Def.port, 0x00) == 0) {
        printf("\a%d : Fail to create socket.", mSocket);
        ch_status[mSocket] = 0;
      }
      
      break;
  }
}

