/* Includes ------------------------------------------------------------------*/
#include "socket.h"

// Send Flag
extern uint8_t SendFlag;

/* Private function prototypes -----------------------------------------------*/
void WIZ820io_SPI2_Configuration(void);
void WIZ820io_SPI3_Configuration(void);
void Set_network(void);
void printSysCfg(void);
uint32_t time_return(void);
void ProcessTcpServer(SOCKET mSocket, uint16 port);
void ProcessTcpClient(SOCKET mSocket);