/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "glcd.h"	// beacause delay_us function

/* Private define ------------------------------------------------------------*/
#define TLCD_RS_HIGH()    GPIO_WriteBit(GPIOB, GPIO_Pin_1, Bit_SET)
#define TLCD_RS_LOW()     GPIO_WriteBit(GPIOB, GPIO_Pin_1, Bit_RESET)
  
#define TLCD_RW_HIGH()    GPIO_WriteBit(GPIOE, GPIO_Pin_8, Bit_SET)
#define TLCD_RW_LOW()     GPIO_WriteBit(GPIOE, GPIO_Pin_8, Bit_RESET)
  
#define TLCD_E_HIGH()     GPIO_WriteBit(GPIOE, GPIO_Pin_7, Bit_SET)
#define TLCD_E_LOW()      GPIO_WriteBit(GPIOE, GPIO_Pin_7, Bit_RESET)

#define RIGHT     1
#define LEFT      0

#define HIGH      1
#define LOW       0

#define RS_HIGH   1
#define RS_LOW    0

/* Private function prototype ------------------------------------------------*/

void TLCD_Configuration(void);
void TLCD_Handler(char rs, char data);
void TLCD_Clear(void);
void TLCD_EntryMode(void);
void TLCD_OnOFF(int OnOff);
void TLCD_Write(char row, char column, char *ch);
void LCD_Shift(char rl);
void TLCD_FunctionSet();