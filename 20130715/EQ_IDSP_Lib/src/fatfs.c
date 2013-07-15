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
char dirPath[30];

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
  
  res = f_getfree(path, &fre_clust, &fs);
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
      printf("FR_OK");
      break;
    case FR_DISK_ERR:
      printf("FR_DISK_ERR");
      break;
    case FR_INT_ERR:
      printf("FR_INT_ERR");
      break;
    case FR_NOT_READY:
      printf("FR_NOT_READY");
      break;
    case FR_NO_FILE:
      printf("FR_NO_FILE");
      break;
    case FR_NO_PATH:
      printf("FR_NO_PATH");
      break;
    case FR_INVALID_NAME:
      printf("FR_INVALID_NAME");
      break;
    case FR_INVALID_DRIVE:
      printf("FR_INVALID_DRIVE");
      break;
    case FR_DENIED:
      printf("FR_DENIED");
      break;
    case FR_EXIST:
      printf("FR_EXIST");
      break;
    case FR_WRITE_PROTECTED:
      printf("FR_WRITE_PROTECTED");
      break;
    case FR_NOT_ENABLED:
      printf("FR_NOT_ENABLED");
      break;
    case FR_NO_FILESYSTEM:
      printf("FR_NO_FILESYSTEM");
      break;
    case FR_INVALID_OBJECT:
      printf("FR_INVALID_OBJECT");
      break;
    case FR_MKFS_ABORTED:
      printf("FR_MKFS_ABORTED");
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

char* CreateDirectoryAccordingly(int year, int month, int day, int hour) {  
  int mYear, mMonth, mDay, mHour;
  mYear = year; mMonth = month; mDay = day; mHour = hour;

  printf("\r\n\r\nf_mkdir(0:/%04d%02d%02d) : ", mYear, mMonth, mDay);
  sprintf(dirPath, "0:/%04d%02d%02d", mYear, mMonth, mDay);
  res = f_mkdir(dirPath);
  FPrintFatResult(res);
  
  switch(mHour) {
  case 0:
    printf("\r\nf_mkdir(0:/%04d%02d%02d/00H-01H) : ", mYear, mMonth, mDay);
    sprintf(dirPath, "0:/%04d%02d%02d/00H-01H", mYear, mMonth, mDay);
    res = f_mkdir(dirPath);
    FPrintFatResult(res);
    break;
  case 1:
    printf("\r\nf_mkdir(0:/%04d%02d%02d/01H-02H) : ", mYear, mMonth, mDay);
    sprintf(dirPath, "0:/%04d%02d%02d/01H-02H", mYear, mMonth, mDay);
    res = f_mkdir(dirPath);
    FPrintFatResult(res);
    break;
  case 2:
    printf("\r\nf_mkdir(0:/%04d%02d%02d/02H-03H) : ", mYear, mMonth, mDay);
    sprintf(dirPath, "0:/%04d%02d%02d/02H-03H", mYear, mMonth, mDay);
    res = f_mkdir(dirPath);
    FPrintFatResult(res);
    break;
  case 3:
    printf("\r\nf_mkdir(0:/%04d%02d%02d/03H-04H) : ", mYear, mMonth, mDay);
    sprintf(dirPath, "0:/%04d%02d%02d/03H-04H", mYear, mMonth, mDay);
    res = f_mkdir(dirPath);
    FPrintFatResult(res);
    break;
  case 4:
    printf("\r\nf_mkdir(0:/%04d%02d%02d/04H-05H) : ", mYear, mMonth, mDay);
    sprintf(dirPath, "0:/%04d%02d%02d/04H-05H", mYear, mMonth, mDay);
    res = f_mkdir(dirPath);
    FPrintFatResult(res);
    break;
  case 5:
    printf("\r\nf_mkdir(0:/%04d%02d%02d/05H-06H) : ", mYear, mMonth, mDay);
    sprintf(dirPath, "0:/%04d%02d%02d/05H-06H", mYear, mMonth, mDay);
    res = f_mkdir(dirPath);
    FPrintFatResult(res);
    break;
  case 6:
    printf("\r\nf_mkdir(0:/%04d%02d%02d/06H-07H) : ", mYear, mMonth, mDay);
    sprintf(dirPath, "0:/%04d%02d%02d/06H-07H", mYear, mMonth, mDay);
    res = f_mkdir(dirPath);
    FPrintFatResult(res);
    break;
  case 7:
    printf("\r\nf_mkdir(0:/%04d%02d%02d/07H-08H) : ", mYear, mMonth, mDay);
    sprintf(dirPath, "0:/%04d%02d%02d/07H-08H", mYear, mMonth, mDay);
    res = f_mkdir(dirPath);
    FPrintFatResult(res);
    break;
  case 8:
    printf("\r\nf_mkdir(0:/%04d%02d%02d/08H-09H) : ", mYear, mMonth, mDay);
    sprintf(dirPath, "0:/%04d%02d%02d/08H-09H", mYear, mMonth, mDay);
    res = f_mkdir(dirPath);
    FPrintFatResult(res);
    break;
  case 9:
    printf("\r\nf_mkdir(0:/%04d%02d%02d/09H-10H) : ", mYear, mMonth, mDay);
    sprintf(dirPath, "0:/%04d%02d%02d/09H-10H", mYear, mMonth, mDay);
    res = f_mkdir(dirPath);
    FPrintFatResult(res);
    break;
  case 10:
    printf("\r\nf_mkdir(0:/%04d%02d%02d/10H-11H) : ", mYear, mMonth, mDay);
    sprintf(dirPath, "0:/%04d%02d%02d/10H-11H", mYear, mMonth, mDay);
    res = f_mkdir(dirPath);
    FPrintFatResult(res);
    break;
  case 11:
    printf("\r\nf_mkdir(0:/%04d%02d%02d/11H-12H) : ", mYear, mMonth, mDay);
    sprintf(dirPath, "0:/%04d%02d%02d/11H-12H", mYear, mMonth, mDay);
    res = f_mkdir(dirPath);
    FPrintFatResult(res);
    break;
  case 12:
    printf("\r\nf_mkdir(0:/%04d%02d%02d/12H-13H) : ", mYear, mMonth, mDay);
    sprintf(dirPath, "0:/%04d%02d%02d/12H-13H", mYear, mMonth, mDay);
    res = f_mkdir(dirPath);
    FPrintFatResult(res);
    break;
  case 13:
    printf("\r\nf_mkdir(0:/%04d%02d%02d/13H-14H) : ", mYear, mMonth, mDay);
    sprintf(dirPath, "0:/%04d%02d%02d/13H-14H", mYear, mMonth, mDay);
    res = f_mkdir(dirPath);
    FPrintFatResult(res);
    break;
  case 14:
    printf("\r\nf_mkdir(0:/%04d%02d%02d/14H-15H) : ", mYear, mMonth, mDay);
    sprintf(dirPath, "0:/%04d%02d%02d/14H-15H", mYear, mMonth, mDay);
    res = f_mkdir(dirPath);
    FPrintFatResult(res);
    break;
  case 15:
    printf("\r\nf_mkdir(0:/%04d%02d%02d/15H-16H) : ", mYear, mMonth, mDay);
    sprintf(dirPath, "0:/%04d%02d%02d/15H-16H", mYear, mMonth, mDay);
    res = f_mkdir(dirPath);
    FPrintFatResult(res);
    break;
  case 16:
    printf("\r\nf_mkdir(0:/%04d%02d%02d/16H-17H) : ", mYear, mMonth, mDay);
    sprintf(dirPath, "0:/%04d%02d%02d/16H-17H", mYear, mMonth, mDay);
    res = f_mkdir(dirPath);
    FPrintFatResult(res);
    break;
  case 17:
    printf("\r\nf_mkdir(0:/%04d%02d%02d/17H-18H) : ", mYear, mMonth, mDay);
    sprintf(dirPath, "0:/%04d%02d%02d/17H-18H", mYear, mMonth, mDay);
    res = f_mkdir(dirPath);
    FPrintFatResult(res);
    break;
  case 18:
    printf("\r\nf_mkdir(0:/%04d%02d%02d/18H-19H) : ", mYear, mMonth, mDay);
    sprintf(dirPath, "0:/%04d%02d%02d/18H-19H", mYear, mMonth, mDay);
    res = f_mkdir(dirPath);
    FPrintFatResult(res);
    break;
  case 19:
    printf("\r\nf_mkdir(0:/%04d%02d%02d/19H-20H) : ", mYear, mMonth, mDay);
    sprintf(dirPath, "0:/%04d%02d%02d/19H-20H", mYear, mMonth, mDay);
    res = f_mkdir(dirPath);
    FPrintFatResult(res);
    break;
  case 20:
    printf("\r\nf_mkdir(0:/%04d%02d%02d/20H-21H) : ", mYear, mMonth, mDay);
    sprintf(dirPath, "0:/%04d%02d%02d/20H-21H", mYear, mMonth, mDay);
    res = f_mkdir(dirPath);
    FPrintFatResult(res);
    break;
  case 21:
    printf("\r\nf_mkdir(0:/%04d%02d%02d/21H-22H) : ", mYear, mMonth, mDay);
    sprintf(dirPath, "0:/%04d%02d%02d/21H-22H", mYear, mMonth, mDay);
    res = f_mkdir(dirPath);
    FPrintFatResult(res);
    break;
  case 22:
    printf("\r\nf_mkdir(0:/%04d%02d%02d/22H-23H) : ", mYear, mMonth, mDay);
    sprintf(dirPath, "0:/%04d%02d%02d/22H-23H", mYear, mMonth, mDay);
    res = f_mkdir(dirPath);
    FPrintFatResult(res);
    break;
  case 23:
    printf("\r\nf_mkdir(0:/%04d%02d%02d/23H-00H) : ", mYear, mMonth, mDay);
    sprintf(dirPath, "0:/%04d%02d%02d/23H-00H", mYear, mMonth, mDay);
    res = f_mkdir(dirPath);
    FPrintFatResult(res);
    break;
  }
  return dirPath;
}

void CreateFileAppendModeAccordingly(char *dirPath, int min) {
  int mMin;
  mMin = min / 3;
  
  char filePath[40];
  strcpy(filePath, dirPath);
  printf("\r\nopen_append(&fsrc, filePath) : ");
  
  switch(mMin) {
  case 0:
    strcat(filePath, "/EQLOG1.TXT");
    // Create log file on the drive 0
    res = open_append(&fsrc, filePath);
    FPrintFatResult(res);
    if(res == FR_OK) {
      // Write buffer to file
      //res = f_write(&fsrc, HEADER, sizeof(HEADER), &br);
      
      printf("\r\nEQLOG1.txt successfully created");
      
      // Close file
      //f_close(&fsrc);
      
    } else if ( res == FR_EXIST ) {
      printf("EQLOG1.txt already exist");
    }
    break;
  case 1:
    strcat(filePath, "/EQLOG2.TXT");
    // Create log file on the drive 0
    res = open_append(&fsrc, filePath);
    FPrintFatResult(res);
    if(res == FR_OK) {
      // Write buffer to file
      //res = f_write(&fsrc, HEADER, sizeof(HEADER), &br);
      
      printf("\r\nEQLOG2.txt successfully created");
      
      // Close file
      //f_close(&fsrc);
      
    } else if ( res == FR_EXIST ) {
      printf("EQLOG2.txt already exist");
    }
    break;
  case 2:
    strcat(filePath, "/EQLOG3.TXT");
    // Create log file on the drive 0
    res = open_append(&fsrc, filePath);
    FPrintFatResult(res);
    if(res == FR_OK) {
      // Write buffer to file
      //res = f_write(&fsrc, HEADER, sizeof(HEADER), &br);
      
      printf("\r\nEQLOG3.txt successfully created");
      
      // Close file
      //f_close(&fsrc);
      
    } else if ( res == FR_EXIST ) {
      printf("EQLOG3.txt already exist");
    }
    break;
  case 3:
    strcat(filePath, "/EQLOG4.TXT");
    // Create log file on the drive 0
    res = open_append(&fsrc, filePath);
    FPrintFatResult(res);
    if(res == FR_OK) {
      // Write buffer to file
      //res = f_write(&fsrc, HEADER, sizeof(HEADER), &br);
      
      printf("\r\nEQLOG4.txt successfully created");
      
      // Close file
      //f_close(&fsrc);
      
    } else if ( res == FR_EXIST ) {
      printf("EQLOG4.txt already exist");
    }
    break;
  case 4:
    strcat(filePath, "/EQLOG5.TXT");
    // Create log file on the drive 0
    res = open_append(&fsrc, filePath);
    FPrintFatResult(res);
    if(res == FR_OK) {
      // Write buffer to file
      //res = f_write(&fsrc, HEADER, sizeof(HEADER), &br);
      
      printf("\r\nEQLOG5.txt successfully created");
      
      // Close file
      //f_close(&fsrc);
      
    } else if ( res == FR_EXIST ) {
      printf("EQLOG5.txt already exist");
    }
    break;
  case 5:
    strcat(filePath, "/EQLOG6.TXT");
    // Create log file on the drive 0
    res = open_append(&fsrc, filePath);
    FPrintFatResult(res);
    if(res == FR_OK) {
      // Write buffer to file
      //res = f_write(&fsrc, HEADER, sizeof(HEADER), &br);
      
      printf("\r\nEQLOG6.txt successfully created");
      
      // Close file
      //f_close(&fsrc);
      
    } else if ( res == FR_EXIST ) {
      printf("EQLOG6.txt already exist");
    }
    break;
  case 6:
    strcat(filePath, "/EQLOG7.TXT");
    // Create log file on the drive 0
    res = open_append(&fsrc, filePath);
    FPrintFatResult(res);
    if(res == FR_OK) {
      // Write buffer to file
      //res = f_write(&fsrc, HEADER, sizeof(HEADER), &br);
      
      printf("\r\nEQLOG7.txt successfully created");
      
      // Close file
      //f_close(&fsrc);
      
    } else if ( res == FR_EXIST ) {
      printf("EQLOG7.txt already exist");
    }
    break;
  case 7:
    strcat(filePath, "/EQLOG8.TXT");
    // Create log file on the drive 0
    res = open_append(&fsrc, filePath);
    FPrintFatResult(res);
    if(res == FR_OK) {
      // Write buffer to file
      //res = f_write(&fsrc, HEADER, sizeof(HEADER), &br);
      
      printf("\r\nEQLOG8.txt successfully created");
      
      // Close file
      //f_close(&fsrc);
      
    } else if ( res == FR_EXIST ) {
      printf("EQLOG8.txt already exist");
    }
    break;
  case 8:
    strcat(filePath, "/EQLOG9.TXT");
    // Create log file on the drive 0
    res = open_append(&fsrc, filePath);
    FPrintFatResult(res);
    if(res == FR_OK) {
      // Write buffer to file
      //res = f_write(&fsrc, HEADER, sizeof(HEADER), &br);
      
      printf("\r\nEQLOG9.txt successfully created");
      
      // Close file
      //f_close(&fsrc);
      
    } else if ( res == FR_EXIST ) {
      printf("EQLOG9.txt already exist");
    }
    break;
  case 9:
    strcat(filePath, "/EQLOG10.TXT");
    // Create log file on the drive 0
    res = open_append(&fsrc, filePath);
    FPrintFatResult(res);
    if(res == FR_OK) {
      // Write buffer to file
      //res = f_write(&fsrc, HEADER, sizeof(HEADER), &br);
      
      printf("\r\nEQLOG10.txt successfully created");
      
      // Close file
      //f_close(&fsrc);
      
    } else if ( res == FR_EXIST ) {
      printf("EQLOG10.txt already exist");
    }
    break;
  case 10:
    strcat(filePath, "/EQLOG11.TXT");
    // Create log file on the drive 0
    res = open_append(&fsrc, filePath);
    FPrintFatResult(res);
    if(res == FR_OK) {
      // Write buffer to file
      //res = f_write(&fsrc, HEADER, sizeof(HEADER), &br);
      
      printf("\r\nEQLOG11.txt successfully created");
      
      // Close file
      //f_close(&fsrc);
      
    } else if ( res == FR_EXIST ) {
      printf("EQLOG11.txt already exist");
    }
    break;
  case 11:
    strcat(filePath, "/EQLOG12.TXT");
    // Create log file on the drive 0
    res = open_append(&fsrc, filePath);
    FPrintFatResult(res);
    if(res == FR_OK) {
      // Write buffer to file
      //res = f_write(&fsrc, HEADER, sizeof(HEADER), &br);
      
      printf("\r\nEQLOG12.txt successfully created");
      
      // Close file
      //f_close(&fsrc);
      
    } else if ( res == FR_EXIST ) {
      printf("EQLOG12.txt already exist");
    }
    break;
  case 12:
    strcat(filePath, "/EQLOG13.TXT");
    // Create log file on the drive 0
    res = open_append(&fsrc, filePath);
    FPrintFatResult(res);
    if(res == FR_OK) {
      // Write buffer to file
      //res = f_write(&fsrc, HEADER, sizeof(HEADER), &br);
      
      printf("\r\nEQLOG13.txt successfully created");
      
      // Close file
      //f_close(&fsrc);
      
    } else if ( res == FR_EXIST ) {
      printf("EQLOG13.txt already exist");
    }
    break;
  case 13:
    strcat(filePath, "/EQLOG14.TXT");
    // Create log file on the drive 0
    res = open_append(&fsrc, filePath);
    FPrintFatResult(res);
    if(res == FR_OK) {
      // Write buffer to file
      //res = f_write(&fsrc, HEADER, sizeof(HEADER), &br);
      
      printf("\r\nEQLOG14.txt successfully created");
      
      // Close file
      //f_close(&fsrc);
      
    } else if ( res == FR_EXIST ) {
      printf("EQLOG14.txt already exist");
    }
    break;
  case 14:
    strcat(filePath, "/EQLOG15.TXT");
    // Create log file on the drive 0
    res = open_append(&fsrc, filePath);
    FPrintFatResult(res);
    if(res == FR_OK) {
      // Write buffer to file
      //res = f_write(&fsrc, HEADER, sizeof(HEADER), &br);
      
      printf("\r\nEQLOG15.txt successfully created");
      
      // Close file
      //f_close(&fsrc);
      
    } else if ( res == FR_EXIST ) {
      printf("EQLOG15.txt already exist");
    }
    break;
  case 15:
    strcat(filePath, "/EQLOG16.TXT");
    // Create log file on the drive 0
    res = open_append(&fsrc, filePath);
    FPrintFatResult(res);
    if(res == FR_OK) {
      // Write buffer to file
      //res = f_write(&fsrc, HEADER, sizeof(HEADER), &br);
      
      printf("\r\nEQLOG16.txt successfully created");
      
      // Close file
      //f_close(&fsrc);
      
    } else if ( res == FR_EXIST ) {
      printf("EQLOG16.txt already exist");
    }
    break;
  case 16:
    strcat(filePath, "/EQLOG17.TXT");
    // Create log file on the drive 0
    res = open_append(&fsrc, filePath);
    FPrintFatResult(res);
    if(res == FR_OK) {
      // Write buffer to file
      //res = f_write(&fsrc, HEADER, sizeof(HEADER), &br);
      
      printf("\r\nEQLOG17.txt successfully created");
      
      // Close file
      //f_close(&fsrc);
      
    } else if ( res == FR_EXIST ) {
      printf("EQLOG17.txt already exist");
    }
    break;
  case 17:
    strcat(filePath, "/EQLOG18.TXT");
    // Create log file on the drive 0
    res = open_append(&fsrc, filePath);
    FPrintFatResult(res);
    if(res == FR_OK) {
      // Write buffer to file
      //res = f_write(&fsrc, HEADER, sizeof(HEADER), &br);
      
      printf("\r\nEQLOG18.txt successfully created");
      
      // Close file
      //f_close(&fsrc);
      
    } else if ( res == FR_EXIST ) {
      printf("EQLOG18.txt already exist");
    }
    break;
  case 18:
    strcat(filePath, "/EQLOG19.TXT");
    // Create log file on the drive 0
    res = open_append(&fsrc, filePath);
    FPrintFatResult(res);
    if(res == FR_OK) {
      // Write buffer to file
      //res = f_write(&fsrc, HEADER, sizeof(HEADER), &br);
      
      printf("\r\nEQLOG19.txt successfully created");
      
      // Close file
      //f_close(&fsrc);
      
    } else if ( res == FR_EXIST ) {
      printf("EQLOG19.txt already exist");
    }
    break;
  case 19:
    strcat(filePath, "/EQLOG20.TXT");
    // Create log file on the drive 0
    res = open_append(&fsrc, filePath);
    FPrintFatResult(res);
    if(res == FR_OK) {
      // Write buffer to file
      //res = f_write(&fsrc, HEADER, sizeof(HEADER), &br);
      
      printf("\r\nEQLOG20.txt successfully created");
      
      // Close file
      //f_close(&fsrc);
      
    } else if ( res == FR_EXIST ) {
      printf("EQLOG20.txt already exist");
    }
    break;
  }
}

