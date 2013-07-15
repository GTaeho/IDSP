/* Includes ------------------------------------------------------------------*/
#include "ff.h"
#include <stdio.h>
#include <string.h>

/* Private functions prototype -----------------------------------------------*/
FRESULT scan_files (char* path);
FRESULT open_append (FIL* fp, const char* path);
int SD_TotalSize(void);
void FPrintFatResult(FRESULT tResult);
char* CreateDirectoryAccordingly(int year, int month, int day, int hour);
void CreateFileAppendModeAccordingly(char *dirPath, int min);
