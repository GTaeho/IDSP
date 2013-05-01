/* GLCD Raw Data ---------------------------------------*/
extern unsigned char intro_logo[1024];
extern unsigned char basicif[1024];
extern unsigned char plot_x[72];
extern unsigned char plot_y[72];
extern unsigned char plot_z[72];
extern unsigned char name_logo[1024];

/* GLCD Graph container */
extern unsigned char mode, i, j, x, y, offset_start, offset_finish;
extern int index, start, flag_uart, tmp_start;
extern int data_x[100], data_y[100], data_z[100], data_g[100];
extern int tmp_data_x_lcd[100], tmp_data_y_lcd[100], tmp_data_z_lcd[100];

/* GLCD macro ----------------------------------------*/
#define GLCD_RS_HIGH()      GPIOD->BSRRL = GPIO_Pin_11
#define GLCD_RS_LOW()       GPIOD->BSRRH = GPIO_Pin_11

#define GLCD_RW_HIGH()      GPIOD->BSRRL = GPIO_Pin_12
#define GLCD_RW_LOW()       GPIOD->BSRRH = GPIO_Pin_12

#define GLCD_ENABLE_HIGH()  GPIOD->BSRRL = GPIO_Pin_13
#define GLCD_ENABLE_LOW()   GPIOD->BSRRH = GPIO_Pin_13

#define GLCD_CS1_HIGH()     GPIOA->BSRRL = GPIO_Pin_10
#define GLCD_CS1_LOW()      GPIOA->BSRRH = GPIO_Pin_10

#define GLCD_CS2_HIGH()     GPIOA->BSRRL = GPIO_Pin_11
#define GLCD_CS2_LOW()      GPIOA->BSRRH = GPIO_Pin_11

#define GLCD_RESET_HIGH()   GPIOA->BSRRL = GPIO_Pin_12
#define GLCD_RESET_LOW()    GPIOA->BSRRH = GPIO_Pin_12


/* GLCD User define -----------------------------------*/
#define RS_READ     0
#define RS_WRITE    1
  
#define CS1         1
#define CS2         2
#define CS12        3
  
#define DISPLAY_ON  0x3f
#define START_LINE  0xc0
  
#define PAGE_SET    0xb8
#define ADDR_SET    0x40

// GLCD function list
void GLCD_Configuration(void);
void GLCD_Handler(char rs, char cs, char data);
void GLCD_Clear(void);
void GLCD_Data(char data);
void GLCD_Intro(void);
void GLCD_BasicIF(void);
void GLCD_CustomIF(void);