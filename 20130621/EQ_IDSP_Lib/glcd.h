/* GLCD Raw Data ---------------------------------------*/
/* GLCD macro ----------------------------------------*/
#define GLCD_RS_HIGH()      GPIO_WriteBit(GPIOC, GPIO_Pin_0, Bit_SET)
#define GLCD_RS_LOW()       GPIO_WriteBit(GPIOC, GPIO_Pin_0, Bit_RESET)

#define GLCD_RW_HIGH()      GPIO_WriteBit(GPIOC, GPIO_Pin_1, Bit_SET)
#define GLCD_RW_LOW()       GPIO_WriteBit(GPIOC, GPIO_Pin_1, Bit_RESET)

#define GLCD_ENABLE_HIGH()  GPIO_WriteBit(GPIOC, GPIO_Pin_2, Bit_SET)
#define GLCD_ENABLE_LOW()   GPIO_WriteBit(GPIOC, GPIO_Pin_2, Bit_RESET)

#define GLCD_CS1_HIGH()     GPIO_WriteBit(GPIOC, GPIO_Pin_3, Bit_SET)
#define GLCD_CS1_LOW()      GPIO_WriteBit(GPIOC, GPIO_Pin_3, Bit_RESET)

#define GLCD_CS2_HIGH()     GPIO_WriteBit(GPIOC, GPIO_Pin_4, Bit_SET)
#define GLCD_CS2_LOW()      GPIO_WriteBit(GPIOC, GPIO_Pin_4, Bit_RESET)

#define GLCD_RESET_HIGH()   GPIO_WriteBit(GPIOC, GPIO_Pin_5, Bit_SET)
#define GLCD_RESET_LOW()    GPIO_WriteBit(GPIOC, GPIO_Pin_5, Bit_RESET)


/* GLCD User define -----------------------------------*/
#define RS_READ     0
#define RS_WRITE    1
  
#define CS1         1
#define CS2         2
#define CS12        3
  
#define DISPLAY_ON  0x3F
#define START_LINE  0xC0
  
#define PAGE_SET    0xB8
#define ADDR_SET    0x40

// GLCD function list
void GLCD_Configuration(void);
void GLCD_Handler(char rs, char cs, uint16_t data);
void GLCD_Clear(void);
void GLCD_Data(char data);
void GLCD_Intro(void);
void GLCD_BasicIF(void);
void GLCD_CustomIF(void);
void SelectAxisX(int index);
void SelectAxisY(int index);
void SelectAxisZ(int index);
void GLCD_AxisViewWithWaveform(int axis, int index);
void GLCD_DisplayKMAIntensity(float data);