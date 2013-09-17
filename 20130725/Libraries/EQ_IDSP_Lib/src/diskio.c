/* Includes ------------------------------------------------------------------*/
#include "diskio.h"
#include "sdcard.h"
#include "fsmc_nand.h"
#include "nand_if.h"

/* Private variables ---------------------------------------------------------*/

extern SD_CardInfo SDCardInfo;

// Time variable updates every second from rtc.c
extern __IO uint32_t THH, TMM, TSS;

DSTATUS disk_initialize (
	BYTE drv    /* Physical drive nmuber (0..) */
)
{
  SD_Error Status;
  switch (drv) {
    case 0 :
      Status = SD_Init();
      SD_GetCardInfo(&SDCardInfo);
      SD_SelectDeselect((uint32_t) (SDCardInfo.RCA << 16));
      SD_EnableWideBusOperation(SDIO_BusWide_4b);
      SD_SetDeviceMode(SD_DMA_MODE);
   
      NVIC_InitTypeDef NVIC_InitStructure;
      NVIC_InitStructure.NVIC_IRQChannel = SDIO_IRQn; 
      NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; 
      NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; 
      NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
      NVIC_Init(&NVIC_InitStructure);
      
      if ( Status == SD_OK ) {
        return RES_OK;
      } else {
        return STA_NOINIT;
      }
    
    case 1 :
      //NAND_Init();
      return RES_OK; 
      
    case 2 : 
      return STA_NOINIT;
  }
  
  return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */

DSTATUS disk_status (
	BYTE drv		/* Physical drive nmuber (0..) */
)
{
  switch (drv) {
    case 0 :
    /* translate the reslut code here	*/
      return RES_OK;
    case 1 :
    /* translate the reslut code here	*/
      return RES_OK;
    case 2 :
    /* translate the reslut code here	*/
      return RES_OK;
    default: break;
  }
  
  return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */

DRESULT disk_read (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	        /* Sector address (LBA) */
	BYTE count		/* Number of sectors to read (1..255) */
)
{ 
  SD_Error Status;
  if ( !count ) {
    return RES_PARERR;
  }

  switch (drv) {
    case 0:
      if(count == 1) {
        Status = SD_ReadBlock( sector<<9 , (uint32_t *)buff, SDCardInfo.CardBlockSize );
      } else {
        Status = SD_ReadMultiBlocks( sector<<9 , (uint32_t *)buff, SDCardInfo.CardBlockSize, count );
      }
      
      if ( Status == SD_OK ) {
        return RES_OK;
      } else {
        return RES_ERROR;
      }
      
    case 1:
      //NAND_Read(Memory_Offset, (uint32_t *)buff, Transfer_Length); 
      break;
      
    case 2:
      break;
      
    default:
      break;

  }
  
  return RES_ERROR;
}


/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */

#if _READONLY == 0
DRESULT disk_write (
	BYTE drv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	        /* Data to be written */
	DWORD sector,		        /* Sector address (LBA) */
	BYTE count			/* Number of sectors to write (1..255) */
)
{
  SD_Error Status;
  if ( !count ) {
    return RES_PARERR;
  }

  switch (drv) {
    case 0:
      if(count==1) {
        Status = SD_WriteBlock( sector<<9, (uint32_t *)buff, SDCardInfo.CardBlockSize );
      } else {
        Status = SD_WriteMultiBlocks( sector<<9, (uint32_t *)buff, SDCardInfo.CardBlockSize, count );
      }
      
      if ( Status == SD_OK ) {
        return RES_OK;
      } else {
        return RES_ERROR;
      }
      
    case 1:
      //NAND_Write(Memory_Offset, (uint32_t *)buff, Transfer_Length); 
     break;
    
    case 2:
      break;
      
    default :
       break;
  }
  
  return RES_ERROR;
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
  if (drv) {    
    return RES_PARERR;
  }
  
  switch(ctrl) {
    case CTRL_SYNC:
      return RES_OK;
      
    case GET_SECTOR_COUNT:
      *( DWORD* )buff = SDCardInfo.CardCapacity / SDCardInfo.CardBlockSize;
      return RES_OK;
      
    case GET_SECTOR_SIZE:
      *( WORD* )buff = SDCardInfo.CardBlockSize;
      return RES_OK;
      
    case GET_BLOCK_SIZE:
      *(WORD*)buff = SDCardInfo.CardBlockSize;
      return RES_OK;
  
    default:
      break;
  }
  
  return RES_PARERR;
}

/*-----------------------------------------------------------------------*/ 
/* Get current time                                                      */ 
/*-----------------------------------------------------------------------*/ 
DWORD get_fattime (void)
{
  int YY, MM, DD;
  YY = GetYearAndMergeToInt();
  MM = GetMonthAndMergeToInt();
  DD = GetDayAndMergeToInt();
  
  int mHour, mMin, mSec;
  mHour = THH; mMin = TMM; mSec = TSS;
  
  return ((YY-1980) << 25)  // Year
  | (MM << 21)       // Month
  | (DD << 16)       // Day
  | (mHour << 11)    // Hour
  | (mMin << 5)      // Min
  | (mSec >> 1)      // Sec
  ; 
}



/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
