/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "w5200.h"
#include "socket.h"
#include "config.h"
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* -- UART4 message buffersize -- */
#define RxBufferSize   0x32
/* ------------------------------- */

/* Parse definition ---- */
enum { False, True };
/* --------------------- */

#define SOCK_TCPS 0

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

// Timer variables
uint16_t PrescalerValue = 0;
volatile uint16_t TimerCount = 0;

// UART4 Messages buffer
char RxBuffer[RxBufferSize];
volatile unsigned char RxCounter = 0;
volatile unsigned char ParseUART4 = False;

// typedef initialize
CONFIG_MSG Config_Msg;
CHCONFIG_TYPE_DEF Chconfig_Type_Def; 

// Configuration Network Information of W5200
uint8 Enable_DHCP = OFF;
uint8 MAC[6] = {0xEC, 0x08, 0xDC, 0x01, 0x02, 0x03};  //MAC Address
uint8 IP[4] = {192, 168, 0, 25};                      //IP Address
uint8 GateWay[4] = {192, 168, 0, 1};                  //Gateway Address
uint8 SubNet[4] = {255, 255, 255, 0};                 //SubnetMask Address

//TX MEM SIZE- SOCKET 0:8KB, SOCKET 1:2KB, SOCKET2-7:1KB
//RX MEM SIZE- SOCKET 0:8KB, SOCKET 1:2KB, SOCKET2-7:1KB
uint8 txsize[MAX_SOCK_NUM] = {2,2,2,2,2,2,2,2};
uint8 rxsize[MAX_SOCK_NUM] = {2,2,2,2,2,2,2,2};

//FOR TCP Client
//Configuration Network Information of TEST PC
uint8 Dest_IP[4] = {192, 168, 11, 3}; //DST_IP Address 
uint16 Dest_PORT = 3000; //DST_IP port

uint8 ch_status[MAX_SOCK_NUM] = { 0, };	/** 0:close, 1:ready, 2:connected */

uint8 TX_BUF[TX_RX_MAX_BUF_SIZE]; // TX Buffer for applications
uint8 RX_BUF[TX_RX_MAX_BUF_SIZE]; // RX Buffer for applications

// RCC Check variable
volatile uint32_t SYSCLK_Freq;
volatile uint32_t HCLK_Freq;
volatile uint32_t PCLK1_Freq;
volatile uint32_t PCLK2_Freq;

// Send Flag
uint8_t SendFlag = False;


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
  
  SYSCLK_Freq = RCC_ClockFreq.SYSCLK_Frequency;
  HCLK_Freq = RCC_ClockFreq.HCLK_Frequency;
  PCLK1_Freq = RCC_ClockFreq.PCLK1_Frequency;
  PCLK2_Freq = RCC_ClockFreq.PCLK2_Frequency;
}

void TIM4_Configuration(void) {
  /* TIM4 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
  
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
  PrescalerValue = (uint16_t) ((SystemCoreClock / 2) / 84) - 1;

  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 1000;
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

void UART4_Configuration(void) {
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  
  /* RCC configuration -------------------------------------------------------*/
  /* Enable GPIOA for UART4 clock*/
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  
  /* USART4 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
  
  /* GPIO configuration ------------------------------------------------------*/
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

  /* UART4 configuration -----------------------------------------------------*/
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

void Delay_us( u8 time_us ) {
  register u8 i;
  register u8 j;
  for( i=0;i<time_us;i++ ) {
    for( j=0;j<5;j++ ) {  // 25CLK
    
      asm("nop");       //1CLK         
      asm("nop");       //1CLK         
      asm("nop");       //1CLK         
      asm("nop");       //1CLK         
      asm("nop");       //1CLK                  
    }      
  } // 25CLK*0.04us=1us
}

void Delay_ms( u16 time_ms ) {
  register u16 i;
  for( i=0;i<time_ms;i++ ) {
    Delay_us(250);
    Delay_us(250);
    Delay_us(250);
    Delay_us(250);
  }
}

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

/**
  * @brief  Configures the nested vectored interrupt controller.
  * @param  None
  * @retval None
  */
void NVIC_Configuration(void) {
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

/* RETARGET PRINTF */
int fputc(int ch, FILE *stream) {
  while(USART_GetFlagStatus(UART4, USART_FLAG_TXE) == RESET);
  
  USART_SendData(UART4, (char) ch);

  return ch;
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

/**
  * @brief  This function handles TIM4 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM4_IRQHandler(void) {
  TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
    
  TimerCount++;
}

/**
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
       
  /* RCC configuration */
  RCC_Configuration();
  
  /* TIM4 configuration */
  TIM4_Configuration();
  
  /* UART4 configuration */
  UART4_Configuration();
  
  /* NVIC configuration */
  NVIC_Configuration();
  
  /* WIZ820io SPI2 configuration */
  WIZ820io_SPI2_Configuration();  // one that is on left one
  
  /* WIZ820io SPI3 configuration */
  //WIZ820io_SPI3_Configuration();  // one that is on right one

  /* W5200 Configuration */
  Set_network();
  
  while(1) {
    if(TimerCount >= 1000) {
      TimerCount = 0;
      
    }
    
    if(ParseUART4) {
      ParseUART4 = False; 
      
      printSysCfg();
    }
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
  while (1) {
    printf("assert_failed");
  }
}
#endif

