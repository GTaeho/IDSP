/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

/* Private define ------------------------------------------------------------*/
#define CLCD_RS_HIGH()    GPIO_WriteBit(GPIOB, GPIO_Pin_3, Bit_SET)
#define CLCD_RS_LOW()     GPIO_WriteBit(GPIOB, GPIO_Pin_3, Bit_RESET)
  
#define CLCD_RW_HIGH()    GPIO_WriteBit(GPIOD, GPIO_Pin_3, Bit_SET)
#define CLCD_RW_LOW()     GPIO_WriteBit(GPIOD, GPIO_Pin_3, Bit_RESET)
  
#define CLCD_E_HIGH()     GPIO_WriteBit(GPIOA, GPIO_Pin_15, Bit_SET)
#define CLCD_E_LOW()      GPIO_WriteBit(GPIOA, GPIO_Pin_15, Bit_RESET)

#define RIGHT     1
#define LEFT      0

#define HIGH      1
#define LOW       0

#define RS_HIGH   1
#define RS_LOW    0

/* Private function prototype ------------------------------------------------*/
void CLCD_Configuration(void);
void CLCD_Handler(char rs, char data);
void CLCD_Clear(void);
void CLCD_EntryMode(void);
void CLCD_OnOFF(int OnOff);
void CLCD_Write(char row, char column, char *ch);
void CLCD_Shift(char rl);
void CLCD_FunctionSet();