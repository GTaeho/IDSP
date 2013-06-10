/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "sdcard.h"
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define BlockSize            512 /* Block Size in Bytes */
#define BufferWordsSize      (BlockSize >> 2)

#define NumberOfBlocks       2  /* For Multi Blocks operation (Read/Write) */
#define MultiBufferWordsSize ((BlockSize * NumberOfBlocks) >> 2)

#define Operate_Block 0

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
SD_CardInfo SDCardInfo;
u32 Buffer_Block_Tx[BufferWordsSize], Buffer_Block_Rx[BufferWordsSize];
u32 Buffer_MultiBlock_Tx[MultiBufferWordsSize], Buffer_MultiBlock_Rx[MultiBufferWordsSize];
//volatile TestStatus EraseStatus = FAILED, TransferStatus1 = FAILED, TransferStatus2 = FAILED;
SD_Error Status = SD_OK;
ErrorStatus HSEStartUpStatus;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

void SDIO_Configuration(void) {
  GPIO_InitTypeDef  GPIO_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  
  /* GPIOC Periph clock enable */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);
  
  /* Configure PC.08, PC.09, PC.10, PC.11, PC.12 pin: D0, D1, D2, D3, CLK pin */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | 
                                GPIO_Pin_11 | GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  /* Configure PD02 pin: CMD pin */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(GPIOD, &GPIO_InitStructure);

  /* Configure the NVIC Preemption Priority Bits */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

  NVIC_InitStructure.NVIC_IRQChannel = SDIO_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

void SDIO_TEST(void) {
  u32 i;
  
  printf("    \r\n\r\n WARNING: THIS PROCESS WILL ERASE THE SD CARD!\r\n Press 'y' to continue.");
	
  /*-------------------------- SD Init ----------------------------- */
  Status = SD_Init();
  printf("    \r\n\r\n01. ----- SD_Init Status:%d\r\n",Status);
  
  if (Status == SD_OK)
  {
    printf("          Initialize SD card successfully!\r\n\r\n");
    /*----------------- Read CSD/CID MSD registers ------------------*/
    Status = SD_GetCardInfo(&SDCardInfo);
    printf("02. ----- SD_GetCardInfo Status:%d\r\n",Status);
  }
  
  if (Status == SD_OK)
  {
    printf("          Get SD card infomation successfully!\r\n          Block size:%x, Card type:%x\r\n\r\n",SDCardInfo.CardBlockSize,SDCardInfo.CardType);  
    /*----------------- Select Card --------------------------------*/
    Status = SD_SelectDeselect((u32) (SDCardInfo.RCA << 16));
    printf("03. ----- SD_SelectDeselect Status:%d\r\n",Status);
  }
  
  if (Status == SD_OK)
  {
    printf("          Select SD card successfully!\r\n\r\n");
    Status = SD_EnableWideBusOperation(SDIO_BusWide_4b);
    printf("04. ----- SD_EnableWideBusOperation Status:%d\r\n",Status);
  }
  
  /*------------------- Block Erase -------------------------------*/
  if (Status == SD_OK)
  {
    printf("          Enable wide bus operation successfully!\r\n\r\n");
    /* Erase NumberOfBlocks Blocks of WRITE_BL_LEN(512 Bytes) */
    Status = SD_Erase(Operate_Block*BlockSize, (Operate_Block+1)*BlockSize);
    printf("05. ----- SD_Erase Status:%d\r\n",Status);
  }
  
  /* Set Device Transfer Mode to DMA */
  if (Status == SD_OK)
  {  
    printf("          Erase block %d successfully!\r\n          All the data is 0x00\r\n\r\n",Operate_Block);
    Status = SD_SetDeviceMode(SD_DMA_MODE);
    printf("06. ----- SD_SetDeviceMode Status:%d\r\n",Status);
  }

  if (Status == SD_OK)
  {
    printf("          Set SD card mode successfully!\r\n\r\n");
    memset(Buffer_MultiBlock_Rx,0xfe,sizeof(Buffer_MultiBlock_Rx));
    Status = SD_ReadMultiBlocks(Operate_Block*BlockSize, Buffer_MultiBlock_Rx, BlockSize, NumberOfBlocks);
    printf("07. ----- SD_ReadMultiBlocks Status:%d\r\n",Status);
  }
  
  if (Status == SD_OK)
  {
	  printf("          Read 2 blocks from block %d sucessfully!\r\n          All the data is:\r\n",Operate_Block);  //karlno add:20100505 for debug
	  for(i=0;i<sizeof(Buffer_MultiBlock_Rx)>>2;i++)
	  {
		  printf("%02x:0x%08x ",i,Buffer_MultiBlock_Rx[i]);
	  }
	  printf("\r\n\r\n");
  }

  /*------------------- Block Read/Write --------------------------*/
  /* Fill the buffer to send */
  memset(Buffer_Block_Tx, 0x88,sizeof(Buffer_Block_Tx));

  if (Status == SD_OK)
  {
    /* Write block of 512 bytes on address 0 */
    Status = SD_WriteBlock(Operate_Block*BlockSize, Buffer_Block_Tx, BlockSize);
    printf("08. ----- SD_WriteBlock Status:%d\r\n",Status);
  }
  
  if (Status == SD_OK)
  {
    printf("          Write block %d successfully!\r\n          All the data is 0x88\r\n\r\n",Operate_Block);  //karlno add:20100505 for debug
    /* Read block of 512 bytes from address 0 */
    Status = SD_ReadBlock(Operate_Block*BlockSize, Buffer_Block_Rx, BlockSize);
    printf("09. ----- SD_ReadBlock Status:%d\r\n",Status);
  }

  if (Status == SD_OK)
  {
	  printf("          Read block %d successfully!\r\n          All the data is:\r\n",Operate_Block);  //karlno add:20100505 for debug
	  for(i=0;i<sizeof(Buffer_Block_Rx)>>2;i++)
	  {
		  printf("%02x:0x%08x ",i,Buffer_Block_Rx[i]);
	  }
	  printf("\r\n\r\n");
  }
  
  /*--------------- Multiple Block Read/Write ---------------------*/
  /* Fill the buffer to send */
  memset(Buffer_MultiBlock_Tx, 0x66, sizeof(Buffer_MultiBlock_Tx));

  if (Status == SD_OK)
  {
    /* Write multiple block of many bytes on address 0 */
    Status = SD_WriteMultiBlocks((Operate_Block+2)*BlockSize, Buffer_MultiBlock_Tx, BlockSize, NumberOfBlocks);
    printf("10. ----- SD_WriteMultiBlocks Status:%d\r\n",Status);
  }
  
  if (Status == SD_OK)
  {
    printf("          Write 2 blocks from block %d successfully!\r\n          All the data is 0x66\r\n\r\n",Operate_Block+2);  //karlno add:20100505 for debug
    /* Read block of many bytes from address 0 */
    Status = SD_ReadMultiBlocks((Operate_Block+2)*BlockSize, Buffer_MultiBlock_Rx, BlockSize, NumberOfBlocks);
    printf("11. ----- SD_ReadMultiBlocks Status:%d\r\n",Status);
  }

  
  if (Status == SD_OK)
  {
    printf("          Read 2 blocks from block %d successfully\r\n          All the data is:\r\n",Operate_Block+2);  //karlno add:20100505 for debug
    for(i=0;i<sizeof(Buffer_MultiBlock_Rx)>>2;i++)
    {
      printf("%02x:0x%08x ",i,Buffer_MultiBlock_Rx[i]);
    }
    printf("\r\n\r\n");
  } 
}
