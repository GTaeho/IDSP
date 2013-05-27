/* Includes ------------------------------------------------------------------*/
#include "fatfs.h"

/* Private variables ---------------------------------------------------------*/
/* FatFs elements */
FATFS fs;         /* Work area (file system object) for logical drive */
FIL fsrc;         /* file objects */
FRESULT res;
UINT br;

// FatFs path
char path[512]="0:";

/* Private functions ---------------------------------------------------------*/

FRESULT scan_files (char* path) {
  FILINFO fno;
  DIR dir;
  int i;
  char *fn;
#if _USE_LFN
  static char lfn[_MAX_LFN * (_DF1S ? 2 : 1) + 1];
  fno.lfname = lfn;
  fno.lfsize = sizeof(lfn);
#endif
  res = f_opendir(&dir, path);
  if (res == FR_OK) {
    i = strlen(path);
    for (;;) {
      res = f_readdir(&dir, &fno);
      if (res != FR_OK || fno.fname[0] == 0) break;
      if (fno.fname[0] == '.') continue;
#if _USE_LFN
      fn = *fno.lfname ? fno.lfname : fno.fname;
#else
      fn = fno.fname;
#endif
      if (fno.fattrib & AM_DIR) {
          sprintf(&path[i], "/%s", fn);
          res = scan_files(path);
          if (res != FR_OK) break;
          path[i] = 0;
      } else {
          printf("\r\n%s/%s", path, fn);
      }
    }
  }

  return res;
}


int SD_TotalSize(void) {
  FATFS *fs;
  DWORD fre_clust;        
  
  res = f_getfree("0:", &fre_clust, &fs);
  printf("\r\n\n=============== mSD SDIO Info ===================");
  printf("\r\n * Total space in SD Card");
  if ( res==FR_OK ) {
    /* Print free space in unit of MB (assuming 512 bytes/sector) */
    printf("\r\n%d MB total drive space.\r\n"
         "%d MB available.\r\n",
         ( (fs->n_fatent - 2) * fs->csize ) / 2 /1024 , (fre_clust * fs->csize) / 2 /1024 );
              
    return ENABLE;
  } else {
    return DISABLE; 
  }
}

void FPrintFatResult(FRESULT tResult) {
  switch(tResult) {
    case FR_OK:
      printf("FR_OK\n");
      break;
    case FR_NOT_READY:
      printf("FR_NOT_READY\n");
      break;
    case FR_NO_FILE:
      printf("FR_NO_FILE\n");
      break;
    case FR_NO_PATH:
      printf("FR_NO_PATH\n");
      break;
    case FR_INVALID_NAME:
      printf("FR_INVALID_NAME\n");
      break;
    case FR_INVALID_DRIVE:
      printf("FR_INVALID_DRIVE\n");
      break;
    case FR_DENIED:
      printf("FR_DENIED\n");
      break;
    case FR_EXIST:
      printf("FR_EXIST\n");
      break;
    case FR_WRITE_PROTECTED:
      printf("FR_WRITE_PROTECTED\n");
      break;
    case FR_NOT_ENABLED:
      printf("FR_NOT_ENABLED\n");
      break;
    case FR_NO_FILESYSTEM:
      printf("FR_NO_FILESYSTEM\n");
      break;
    case FR_INVALID_OBJECT:
      printf("FR_INVALID_OBJECT\n");
      break;
    case FR_MKFS_ABORTED:
      printf("FR_MKFS_ABORTED\n");
      break;
  }
}

/*------------------------------------------------------------/
/ Open or create a file in append mode
/------------------------------------------------------------*/
FRESULT open_append (
    FIL* fp,            /* [OUT] File object to create */
    const char* path    /* [IN]  File name to be opened */
)
{
    FRESULT fr;

    /* Opens an existing file. If not exist, creates a new file. */
    fr = f_open(fp, path, FA_WRITE | FA_OPEN_ALWAYS);
    if (fr == FR_OK) {
        /* Seek to end of the file to append data */
        fr = f_lseek(fp, f_size(fp));
        if (fr != FR_OK)
            f_close(fp);
    }
    return fr;
}

