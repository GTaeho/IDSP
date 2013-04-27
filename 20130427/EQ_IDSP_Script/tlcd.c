/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "tlcd.h"
#include "glcd.h"

/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/  
/* Private functions ---------------------------------------------------------*/

void TLCD_Configuration(void) {
  GPIO_InitTypeDef GPIO_InitStructure;
  
  /* RCC cHIGHfiguratiHIGH -------------------------------------------------------*/
  /* Enable TLCD pin */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOE, ENABLE);
  
  /* GPIO cHIGHfiguratiHIGH ------------------------------------------------------*/
  /* RS, DB07 pin configuration */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  /* E, RW, DB0, DB1, DB2, DB3, DB4, DB5, DB6 pin configuration */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_10 | GPIO_Pin_9 | GPIO_Pin_12 | 
                                 GPIO_Pin_11 | GPIO_Pin_14 | GPIO_Pin_13 | GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOE, &GPIO_InitStructure);
  
  /* TLCD initialization */
  Delay_ms(15); // wait before tlcd to be activated
  TLCD_FunctionSet(LOW);    Delay_us(40);
  TLCD_OnOFF(HIGH);         Delay_us(1);
  TLCD_EntryMode();         Delay_us(1);
  TLCD_Clear();             Delay_us(1);
}

void TLCD_Handler(char rs, char data) {
  switch(rs) {
  case RS_HIGH: TLCD_RS_HIGH(); break;
  case RS_LOW: TLCD_RS_LOW(); break;
  default: Delay_ms(1); break;
  }
  
  TLCD_RW_LOW();
  Delay_ms(1);
  
  TLCD_E_HIGH();
  Delay_ms(1);
  
  GPIO_WriteBit(GPIOE, GPIO_Pin_10, (data & 0x01) == Bit_SET ? Bit_SET : Bit_RESET);  // D0
  GPIO_WriteBit(GPIOE, GPIO_Pin_9, ((data & 0x02) >> 1) == Bit_SET ? Bit_SET : Bit_RESET);  // D1
  GPIO_WriteBit(GPIOE, GPIO_Pin_12, ((data & 0x04) >> 2) == Bit_SET ? Bit_SET : Bit_RESET);  // D2
  GPIO_WriteBit(GPIOE, GPIO_Pin_11, ((data & 0x08) >> 3) == Bit_SET ? Bit_SET : Bit_RESET);  // D3
  GPIO_WriteBit(GPIOE, GPIO_Pin_14, ((data & 0x10) >> 4) == Bit_SET ? Bit_SET : Bit_RESET);  // D4
  GPIO_WriteBit(GPIOE, GPIO_Pin_13, ((data & 0x20) >> 5) == Bit_SET ? Bit_SET : Bit_RESET);  // D5
  GPIO_WriteBit(GPIOE, GPIO_Pin_15, ((data & 0x40) >> 6) == Bit_SET ? Bit_SET : Bit_RESET);  // D6
  GPIO_WriteBit(GPIOB, GPIO_Pin_11, ((data & 0x80) >> 7) == Bit_SET ? Bit_SET : Bit_RESET);  // D7
  Delay_us(1);
  
  TLCD_E_LOW();
  Delay_ms(1);
  
  TLCD_E_HIGH();
  Delay_ms(1);
}

void TLCD_Clear(void){
  TLCD_Handler(RS_LOW, 0x01);
}

void TLCD_EntryMode(void){
  TLCD_Handler(RS_LOW, 0x02);
}

void TLCD_OnOFF(int OnOff) {
  if(OnOff == HIGH){
    // Set display, cursor, no blinking of cursor
    TLCD_Handler(RS_LOW, 0x0E);
  } else {
    TLCD_Handler(RS_LOW, 0x08);
  }
}

void TLCD_Write(char row, char column, char *ch){
  if(column == 0) {
    TLCD_Handler(RS_LOW, row + 0x80);
  } else {
    TLCD_Handler(RS_LOW, row + 0xC0);
  }
  
  while(*ch){
    TLCD_Handler(RS_HIGH, *ch++);
  }
}

void LCD_Shift(char rl){
  if(rl == RIGHT) TLCD_Handler(RS_LOW, 0x1C);
  else TLCD_Handler(RS_LOW, 0x18);
} 

void TLCD_FunctionSet(int nf){
  if(nf == LOW) {
    TLCD_Handler(RS_LOW, 0x30);
  } else {
   TLCD_Handler(RS_LOW, 0x3C); 
  }
}

