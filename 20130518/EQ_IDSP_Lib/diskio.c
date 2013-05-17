/* Includes ------------------------------------------------------------------*/
#include "diskio.h"
#include "sdcard.h"

/* Private variables ---------------------------------------------------------*/

extern SD_CardInfo SDCardInfo;

DSTATUS disk_initialize (
	BYTE drv    /* Physical drive nmuber (0..) */
)
{
  NVIC_InitTypeDef NVIC_InitStructure;
  
  switch (drv) {
    case 0 :
    SD_Init();
    SD_GetCardInfo(&SDCardInfo);
    SD_SelectDeselect((uint32_t) (SDCardInfo.RCA << 16));
    SD_EnableWideBusOperation(SDIO_BusWide_4b);
    SD_SetDeviceMode(SD_DMA_MODE);
    NVIC_InitStructure.NVIC_IRQChannel = SDIO_IRQn; 
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; 
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; 
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
    NVIC_Init(&NVIC_InitStructure);     

    //NAND_Init(); 
    
    return 0;
	
    case 1 : return STA_NOINIT; 
    case 2 : return STA_NOINIT;
  }
  
  return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */

DSTATUS disk_status (
	BYTE drv		/* Physical drive nmuber (0..) */
)
{
    switch (drv) 
	{
	  case 0 :
		
	  /* translate the reslut code here	*/

	    return 0;

	  case 1 :
	
	  /* translate the reslut code here	*/

	    return 0;

	  case 2 :
	
	  /* translate the reslut code here	*/

	    return 0;

	  default:

        break;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */

DRESULT disk_read (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	BYTE count		/* Number of sectors to read (1..255) */
)
{
  uint16_t Transfer_Length; 
  uint32_t Memory_Offset; 
  
  Transfer_Length =  count * 512; 
  Memory_Offset = sector * 512; 

  switch (drv) {
    case 0:
      SD_ReadBlock(Memory_Offset, (uint32_t *)buff, Transfer_Length);
      //NAND_Read(Memory_Offset, (uint32_t *)buff, Transfer_Length); 
      break;
    case 1:
      break;
    case 2:
      break;
    default:
      break;

  }
  
  return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */

#if _READONLY == 0
DRESULT disk_write (
	BYTE drv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	        /* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	BYTE count			/* Number of sectors to write (1..255) */
)
{
  uint16_t Transfer_Length; 
  uint32_t Memory_Offset; 
  
  Transfer_Length =  count * 512; 
  Memory_Offset = sector * 512;
  
  switch (drv) {
    case 0:
      SD_WriteBlock(Memory_Offset, (uint32_t *)buff, Transfer_Length); 
      //NAND_Write(Memory_Offset, (uint32_t *)buff, Transfer_Length); 
      break;
    case 2:
      break;
      
    default :
       break;
  }
 return RES_OK;
}
#endif /* _READONLY */



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */

DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
  DRESULT res = RES_OK; 
  uint32_t status = SD_NO_TRANSFER; 
  //uint32_t status = NAND_READY;
  
  switch (ctrl) { 
    case CTRL_SYNC : /// Make sure that no pending write process 
      status = SD_GetTransferState(); 
      if (status == SD_NO_TRANSFER) 
      //status = FSMC_NAND_GetStatus(); 
      //if (status == NAND_READY) 
      {res = RES_OK;} 
      else{res = RES_ERROR;} 
    break; 
    
    case GET_SECTOR_COUNT :   // Get number of sectors on the disk (DWORD) 
      *(DWORD*)buff = 131072; // 4*1024*32 = 131072 
      res = RES_OK; 
    break; 
    
    case GET_SECTOR_SIZE :   // Get R/W sector size (WORD)  
      *(WORD*)buff = 512; 
      res = RES_OK; 
    break; 
    
    case GET_BLOCK_SIZE :     // Get erase block size in unit of sector (DWORD) 
      *(DWORD*)buff = 32; 
      res = RES_OK; 
    } 
     
  return res;   
}

/*-----------------------------------------------------------------------*/ 
/* Get current time                                                      */ 
/*-----------------------------------------------------------------------*/ 
DWORD get_fattime (void)
{
  return ((2006UL-1980) << 25)       // Year = 2006 
  | (2UL << 21)       // Month = Feb 
  | (9UL << 16)       // Day = 9 
  | (22U << 11)       // Hour = 22 
  | (30U << 5)       // Min = 30 
  | (0U >> 1)       // Sec = 0 
  ; 
}



/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
