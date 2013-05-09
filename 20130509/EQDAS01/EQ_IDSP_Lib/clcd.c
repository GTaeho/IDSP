/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"
#include "clcd.h"

/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/  
/* Private functions ---------------------------------------------------------*/

void CLCD_Configuration(void) {
  GPIO_InitTypeDef GPIO_InitStructure;
  
  /* RCC Configuration -----------------------------------------------------------*/
  /* Enable CLCD pin */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE, ENABLE);
  
  /* GPIO Configuration ------------------------------------------------------*/
  /* RS, RW, E pin configuration */  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  
  /* DB0, DB1, DB2, DB3, DB4, DB5, DB6, DB7 pin configuration */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | 
                                 GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
  GPIO_Init(GPIOE, &GPIO_InitStructure);
  
  /* CLCD initialization */
  delay_ms(30); // wait before CLCD to be activated
  CLCD_FunctionSet();       delay_us(50);
  CLCD_OnOFF(HIGH);         delay_us(50);
  CLCD_Clear();             delay_ms(2);
  CLCD_EntryMode();         delay_ms(1);
  CLCD_Write(0, 0, "- IDSP PROJECT -");
  CLCD_Write(0, 1, "- LOADING SYS. -");
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
  
  GPIO_Write(GPIOE, data << 7); // DB0 at PE7
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

