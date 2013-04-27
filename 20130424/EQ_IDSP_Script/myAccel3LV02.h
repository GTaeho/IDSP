//myAccel3LV02 레지스터 주소 정의
#define WHO_AM_I   (0x000f)
#define OFFSET_X   (0x0016)
#define OFFSET_Y   (0x0017)
#define OFFSET_Z   (0x0018)
#define GAIN_X     (0x0019)
#define GAIN_Y     (0x001a)
#define GAIN_Z     (0x001b)
#define CTRL_REG1  (0x0020)
#define CTRL_REG2  (0x0021)
#define CTRL_REG3  (0x0022)
#define HP_FILTER RESET  (0x0023)
#define STATUS_REG (0x0027)
#define OUTX_L     (0x0028)
#define OUTX_H     (0x0029)
#define OUTY_L     (0x002a)
#define OUTY_H     (0x002b)
#define OUTZ_L     (0x002c)
#define OUTZ_H     (0x002d)
#define FF_WU_CFG  (0x0030)
#define FF_WU_SRC  (0x0031)
#define FF_WU_ACK  (0x0032)
#define FF_WU_THS_L  (0x0034)
#define FF_WU_THS_H  (0x0035)
#define FF_WU_DURATION  (0x0036)
#define DD_CFG     (0x0038)
#define DD_SRC     (0x0039)
#define DD_ACK     (0x003a)
#define DD_THSI_L  (0x003c)
#define DD_THSI_H  (0x003d)
#define DD_THSE_L  (0x003e)
#define DD_THSE_H  (0x003f)

#define AXIS_X					0
#define AXIS_Y					1
#define AXIS_Z					2

void Accel3LV02_Configuration(void);
void SPI1_CS_LOW(void);
void SPI1_CS_HIGH(void);
uint8_t SPI1_SendByte(uint8_t byte);

unsigned char Accel_ReadReg(unsigned char reg);
void Accel_WriteReg(unsigned char reg, unsigned char data);
void GetAccelValue(unsigned char Axis, unsigned short *data);