/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "myAccel3LV02.h"

/* Private define ------------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

void Accel3LV02_Configuration(void) {
  GPIO_InitTypeDef GPIO_InitStructure;
  SPI_InitTypeDef  SPI_InitStructure;

  /* RCC configuration -------------------------------------------------------*/  
  /* Enable GPIOA for SPI1 clock */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  
  /* Enable the SPI1 clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
  
  /* GPIO configuration ------------------------------------------------------*/
  /* Connect SPI pins to AFIO */
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1);  // SPI1_SCK
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_SPI1);   // SPI1_MISO
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI1);   // SPI1_MOSI

  /* SPI SCK pin configuration */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* SPI  MISO pin configuration */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* SPI  MOSI pin configuration */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* SPI  NSS pin configuration */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_4;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* SPI configuration -------------------------------------------------------*/
  SPI_I2S_DeInit(SPI1);
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  SPI_Init(SPI1, &SPI_InitStructure);
  
  /* Enable the SPI peripheral */
  SPI_Cmd(SPI1, ENABLE);
}


void SPI1_CS_LOW(void) {
  GPIO_ResetBits(GPIOA, GPIO_Pin_4);
}

void SPI1_CS_HIGH(void) {
  GPIO_SetBits(GPIOA, GPIO_Pin_4);
}

uint8_t SPI1_SendByte(uint8_t byte) {
  /* Loop while DR register in not emplty */
  while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
  
  /* Send byte through the SPI1 peripheral */
  SPI_I2S_SendData(SPI1, byte);
  
  /* Wait to receive a byte */
  while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
  
  /* Return the byte read from the SPI bus */
  return SPI_I2S_ReceiveData(SPI1);
}

//myAccel3LV02의 특정 레지스터에서 데이터를 읽어옴
unsigned char Accel_ReadReg(unsigned char reg) {	
  uint8_t ret = 0;
  
  // CS를 low로 떨어트려서 SPI 통신 시작.
  SPI1_CS_LOW();

  /* 
   * register address를 송신. Read일 경우 최상의 비트는 High
   * 송신 완료될때까지 대기.
   * 수신된 데이터를 저장. 이 데이터는 더미데이터 이다. 
   */
  ret = SPI1_SendByte(0x80|reg);
  
  /*
   * 더미 데이터를 송신.
   * 송신 완료될때까지 대기.
   * 수신된 data byte를 저장.
   */
  ret = SPI1_SendByte(0);
  
  // CS를 high로 올려주면서 SPI 통신 종료.
  SPI1_CS_HIGH();
  
  return ret;
}

//myAccel3LV02의 특정 레지스터에 데이터를 기록
void Accel_WriteReg(unsigned char reg, unsigned char data) {
  // CS를 low로 떨어트려서 SPI 통신 시작.
  SPI1_CS_LOW();
  
  // register address를 송신.
  // 송신 완료될때까지 대기.
  SPI1_SendByte(reg);
  
  // data 송신.
  // 송신 완료될때까지 대기.
  SPI1_SendByte(data);
  
  // CS를 high로 올려주면서 SPI 통신 종료.
  SPI1_CS_HIGH();
}

//myAccel3LV02의 각축의 가속도 값을 읽어옴
void GetAccelValue(unsigned char Axis, unsigned short *data) {
  unsigned char ret;

  while(1) {
    ret = Accel_ReadReg(STATUS_REG); 
    if ((ret & 0x8) != 0)break;
  }
      
  switch(Axis) {
  case AXIS_X:
    *data = Accel_ReadReg(OUTX_L);
    *data |= Accel_ReadReg(OUTX_H)<<8;
    break;
          
  case AXIS_Y:
    *data = Accel_ReadReg(OUTY_L);
    *data |= Accel_ReadReg(OUTY_H)<<8;
    break;

  case AXIS_Z:
    *data = Accel_ReadReg(OUTZ_L);
    *data |= (Accel_ReadReg(OUTZ_H)<<8);
    break;

  default:
    *data = 0;
    break;
  }
}

