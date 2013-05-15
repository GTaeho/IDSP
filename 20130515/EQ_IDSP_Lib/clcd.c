/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"
#include "clcd.h"

/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/  
/* Private functions ---------------------------------------------------------*/

void CLCD_Configuration(void) {
  GPIO_InitTypeDef GPIO_InitStructure;
  
  /* Pin Configuration -----------------------------------------------------------*/
  /* CLCD Pinmap
   * 1. VSS - GND       6. E - PE4          11. DB4 - PE11
   * 2. VDD - 3.3V      7. DB0 - PE5        12. DB5 - PE12
   * 3. V0 - GND        8. DB1 - PE6        13. DB6 - PE13
   * 4. RS - PE2        9. DB2 - PC6        14. DB7 - PE14
   * 5. R/W - PE3       10. DB3 - PC7
  */
  
  /* RCC Configuration -----------------------------------------------------------*/
  /* Enable CLCD pin */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOE, ENABLE);
  
  /* GPIO Configuration ------------------------------------------------------*/
  /* RS, RW, E, DB0, DB1 pin configuration */  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | 
                                GPIO_Pin_5 | GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOE, &GPIO_InitStructure);
  
  /* DB2, DB3 pin configuration */  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  /* DB4, DB5, DB6, DB7 pin configuration */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOE, &GPIO_InitStructure);
  
  /* CLCD initialization */
  delay_ms(30); // wait before CLCD to be activated
  CLCD_FunctionSet();       delay_us(50);
  CLCD_OnOFF(HIGH);         delay_us(50);
  CLCD_Clear();             delay_ms(2);
  CLCD_EntryMode();         delay_ms(1);
  //CLCD_Write(0, 0, "- IDSP PROJECT -");
  //CLCD_Write(0, 1, "- LOADING SYS. -");
  CLCD_Write(0, 1, "jjjjjjjjjjjjjjjj");
}

void CLCD_Handler(char rs, char data) {
  switch(rs) {
  case RS_HIGH: CLCD_RS_HIGH(); break;
  case RS_LOW: CLCD_RS_LOW(); break;
  }
  
  CLCD_RW_LOW();
  delay_us(1);
  
  CLCD_E_HIGH();
  delay_us(500);
  
  GPIO_WriteBit(GPIOE, GPIO_Pin_5, (data & 0x01) == Bit_SET ? Bit_SET : Bit_RESET);  // D0
  GPIO_WriteBit(GPIOE, GPIO_Pin_6, ((data & 0x02) >> 1) == Bit_SET ? Bit_SET : Bit_RESET);  // D1
  GPIO_WriteBit(GPIOC, GPIO_Pin_6, ((data & 0x04) >> 2) == Bit_SET ? Bit_SET : Bit_RESET);  // D2
  GPIO_WriteBit(GPIOC, GPIO_Pin_7, ((data & 0x08) >> 3) == Bit_SET ? Bit_SET : Bit_RESET);  // D3
  GPIO_WriteBit(GPIOE, GPIO_Pin_11, ((data & 0x10) >> 4) == Bit_SET ? Bit_SET : Bit_RESET);  // D4
  GPIO_WriteBit(GPIOE, GPIO_Pin_12, ((data & 0x20) >> 5) == Bit_SET ? Bit_SET : Bit_RESET);  // D5
  GPIO_WriteBit(GPIOE, GPIO_Pin_13, ((data & 0x40) >> 6) == Bit_SET ? Bit_SET : Bit_RESET);  // D6
  GPIO_WriteBit(GPIOE, GPIO_Pin_14, ((data & 0x80) >> 7) == Bit_SET ? Bit_SET : Bit_RESET);  // D7
  delay_us(1);
  
  CLCD_E_LOW();
  delay_us(500);
  
  CLCD_E_HIGH();
  delay_us(1);
}

void CLCD_Clear(void){
  CLCD_Handler(RS_LOW, 0x01);
}

void CLCD_EntryMode(void){
  CLCD_Handler(RS_LOW, 0x06);
}

void CLCD_OnOFF(int OnOff) {
  if(OnOff == HIGH){
    // Set display, cursor, no blinking of cursor
    CLCD_Handler(RS_LOW, 0x0F);
  } else {
    CLCD_Handler(RS_LOW, 0x08);
  }
}

void CLCD_Write(char row, char column, char *ch){
  if(column == 0) {
    CLCD_Handler(RS_LOW, row + 0x80);
  } else {
    CLCD_Handler(RS_LOW, row + 0xC0);
  }
  
  while(*ch){
    CLCD_Handler(RS_HIGH, *ch++);
  }
}

void CLCD_Shift(char rl) {
  if(rl == RIGHT) CLCD_Handler(RS_LOW, 0x1C);
  else CLCD_Handler(RS_LOW, 0x18);
} 

void CLCD_FunctionSet(){
  CLCD_Handler(RS_LOW, 0x3C);
}

