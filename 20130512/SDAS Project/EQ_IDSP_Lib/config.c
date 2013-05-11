/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f10x_it.h"
#include "config.h"
#include "Types.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Text Parser delimiter */
typedef enum { EQ_ONE, EQ_TWO, PC_Cli }BDELIMITER;

/* Private variables ---------------------------------------------------------*/

// AxisData Buffer typedef struct from stm32f4xx_it.h
extern AXISDATA mAxisData;
extern AXISBUF  mAxisBuf;

// GAL container
double tmp_gal = 0;
unsigned short max_gal = 0;

// GAL Buffer originally called here
GALBUF   mGalBuf;

// create 10 second enough data to retain from config.h
EQ_DAQ_ONE DAQBoardOne[1000];
EQ_DAQ_ONE DAQBoardTwo[1000];

/* Private functions ---------------------------------------------------------*/
void ProcessTextStream(uint8_t which, char *string) {
  char *TokenOne = NULL;
  char *TokenTwo = NULL;
  
  switch(which) {
  case EQ_ONE :
    TokenOne = strtok(string, "_"); // result contains Date. ex) 20130428
    DAQBoardOne[0].Date = TokenOne;
    TokenOne = strtok(NULL, "_"); // result contains Time. ex) 21384729
    DAQBoardOne[0].Time = TokenOne;
    TokenOne = strtok(NULL, "_"); // result contains Axis X Data. ex) +4096
    DAQBoardOne[0].AxisX = TokenOne;
    TokenOne = strtok(NULL, "_"); // result contains Axis Y Data. ex) +4096
    DAQBoardOne[0].AxisY = TokenOne;
    TokenOne = strtok(NULL, "_"); // result contains Axis Z Data. ex) +4096
    DAQBoardOne[0].AxisZ = TokenOne;
    
    // DEBUG
    printf("\r\n--------------------------------");
    printf("\r\nEQ_ONE, Date : %s", DAQBoardOne[0].Date);
    printf("\r\nEQ_ONE, Time : %s", DAQBoardOne[0].Time);
    printf("\r\nEQ_ONE, AxisX : %s", DAQBoardOne[0].AxisX);
    printf("\r\nEQ_ONE, AxisY : %s", DAQBoardOne[0].AxisY);
    printf("\r\nEQ_ONE, AxisZ : %s", DAQBoardOne[0].AxisZ);
    break;
    
  case EQ_TWO :
    TokenTwo = strtok(string, "_"); // result contains Date. ex) 20130428
    DAQBoardTwo[0].Date = TokenTwo;
    TokenTwo = strtok(NULL, "_"); // result contains Time. ex) 21384729
    DAQBoardTwo[0].Time = TokenTwo;
    TokenTwo = strtok(NULL, "_"); // result contains Axis X Data. ex) +4096
    DAQBoardTwo[0].AxisX = TokenTwo;
    TokenTwo = strtok(NULL, "_"); // result contains Axis Y Data. ex) +4096
    DAQBoardTwo[0].AxisY = TokenTwo;
    TokenTwo = strtok(NULL, "_"); // result contains Axis Z Data. ex) +4096
    DAQBoardTwo[0].AxisZ = TokenTwo;
    break;
  case PC_Cli: break;
  default: break;
  }
}

// Copy untrimmed data to GAL array with its crude data trimmed
void CopyToTmpGalArray(uint8_t index) {
  if(mAxisBuf.tmp_data_x_lcd[index] & 0x2000)
    mGalBuf.tmp_gal_x[index] = (~mAxisBuf.tmp_data_x_lcd[index] + 1) & 0x1FFF;
  else
    mGalBuf.tmp_gal_x[index] = mAxisBuf.tmp_data_x_lcd[index] & 0x1FFF;
    
  if(mAxisBuf.tmp_data_y_lcd[index] & 0x2000)
    mGalBuf.tmp_gal_y[index] = (~mAxisBuf.tmp_data_y_lcd[index] + 1) & 0x1FFF;
  else
    mGalBuf.tmp_gal_y[index] = mAxisBuf.tmp_data_y_lcd[index] & 0x1FFF;
  
  if(mAxisBuf.tmp_data_z_lcd[index] & 0x2000)
    mGalBuf.tmp_gal_z[index] = (~mAxisBuf.tmp_data_z_lcd[index] + 1) & 0x1FFF;
  else
    mGalBuf.tmp_gal_z[index] = mAxisBuf.tmp_data_z_lcd[index] & 0x1FFF;
}

// According to theory, Maximum G covers 2Gs. so we need to cut off surplus 1G
// there exist in both +, - area.
void CutOffTo1G(uint8_t index) {
  /* Axis X --------------------------------------*/
  //when it is positive value
  if(!(mAxisBuf.tmp_data_x_lcd[index] & 0x2000)) {
    if(mAxisBuf.tmp_data_x_lcd[index] & 0x1000) {
      mAxisBuf.tmp_data_x_lcd[index] = 0x0FFF;
    }
  } else { //when it is negative value
    if(!(mAxisBuf.tmp_data_x_lcd[index] & 0x1000)) {
      mAxisBuf.tmp_data_x_lcd[index] = 0x3001;
    }
  }
  
  /* Axis Y --------------------------------------*/
  if(!(mAxisBuf.tmp_data_y_lcd[index] & 0x2000)) {
    if(mAxisBuf.tmp_data_y_lcd[index] & 0x1000) {
      mAxisBuf.tmp_data_y_lcd[index] = 0x0FFF;
    }
  } else {
    if(!(mAxisBuf.tmp_data_y_lcd[index] & 0x1000)) {
      mAxisBuf.tmp_data_y_lcd[index] = 0x3001;
    }
  }
  
  /* Axis Z --------------------------------------*/
  if(!(mAxisBuf.tmp_data_z_lcd[index] & 0x2000)) {
    if(mAxisBuf.tmp_data_z_lcd[index] & 0x1000) {
      mAxisBuf.tmp_data_z_lcd[index] = 0x0FFF;
    }
  } else {
    if(!(mAxisBuf.tmp_data_z_lcd[index] & 0x1000)) {
      mAxisBuf.tmp_data_z_lcd[index] = 0x3001;
    }
  }  
}

void CalculateGalAndCopyToGal(uint8_t index) {
  // Calculate GAL
  tmp_gal = sqrt(((mGalBuf.tmp_gal_x[index]/4) * (mGalBuf.tmp_gal_x[index]/4)) + 
                 ((mGalBuf.tmp_gal_y[index]/4) * (mGalBuf.tmp_gal_y[index]/4)) + 
                 ((mGalBuf.tmp_gal_z[index]/4) * (mGalBuf.tmp_gal_z[index]/4)));
  
  // Copy to GAL Array
  mAxisData.data_g[index] = (int)tmp_gal;
}

// Determine KMA scale
void DetermineKMA(uint8_t index) {
  if(mAxisData.data_g[index+1] > mAxisData.data_g[index]) {
    max_gal = mAxisData.data_g[index+1];
  } else {
    max_gal = max_gal;
  }
}