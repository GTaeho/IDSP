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

// 10 seconds enough data to retain which from config.h
EQ_DAQ_ONE DAQBoardOne[100];
EQ_DAQ_ONE DAQBoardTwo[100];

/* Private functions ---------------------------------------------------------*/

// Copy untrimmed data to GAL array with its crude data trimmed
void CopyToTmpGalArray(uint8_t index) {
  // for code efficiency
  uint8_t arrIdx = index;
  
  if(mAxisBuf.tmp_data_x_lcd[arrIdx] & 0x2000)
    mGalBuf.tmp_gal_x[arrIdx] = (~mAxisBuf.tmp_data_x_lcd[arrIdx] + 1) & 0x1FFF;
  else
    mGalBuf.tmp_gal_x[arrIdx] = mAxisBuf.tmp_data_x_lcd[arrIdx] & 0x1FFF;
    
  if(mAxisBuf.tmp_data_y_lcd[arrIdx] & 0x2000)
    mGalBuf.tmp_gal_y[arrIdx] = (~mAxisBuf.tmp_data_y_lcd[arrIdx] + 1) & 0x1FFF;
  else
    mGalBuf.tmp_gal_y[arrIdx] = mAxisBuf.tmp_data_y_lcd[arrIdx] & 0x1FFF;
  
  if(mAxisBuf.tmp_data_z_lcd[arrIdx] & 0x2000)
    mGalBuf.tmp_gal_z[arrIdx] = (~mAxisBuf.tmp_data_z_lcd[arrIdx] + 1) & 0x1FFF;
  else
    mGalBuf.tmp_gal_z[arrIdx] = mAxisBuf.tmp_data_z_lcd[arrIdx] & 0x1FFF;
}

// According to theory, Maximum G covers 2Gs. so we need to cut off surplus 1G
// there exist in both +, - area.
void CutOffTo1G(uint8_t index) {
  // for code efficiency
  uint8_t arrIdx = index;
  
  /* Axis X --------------------------------------*/
  //when it is positive value
  if(!(mAxisBuf.tmp_data_x_lcd[arrIdx] & 0x2000)) {
    if(mAxisBuf.tmp_data_x_lcd[arrIdx] & 0x1000) {
      mAxisBuf.tmp_data_x_lcd[arrIdx] = 0x0FFF;
    }
  } else { //when it is negative value
    if(!(mAxisBuf.tmp_data_x_lcd[arrIdx] & 0x1000)) {
      mAxisBuf.tmp_data_x_lcd[arrIdx] = 0x3001;
    }
  }
  
  /* Axis Y --------------------------------------*/
  if(!(mAxisBuf.tmp_data_y_lcd[arrIdx] & 0x2000)) {
    if(mAxisBuf.tmp_data_y_lcd[arrIdx] & 0x1000) {
      mAxisBuf.tmp_data_y_lcd[arrIdx] = 0x0FFF;
    }
  } else {
    if(!(mAxisBuf.tmp_data_y_lcd[arrIdx] & 0x1000)) {
      mAxisBuf.tmp_data_y_lcd[arrIdx] = 0x3001;
    }
  }
  
  /* Axis Z --------------------------------------*/
  if(!(mAxisBuf.tmp_data_z_lcd[arrIdx] & 0x2000)) {
    if(mAxisBuf.tmp_data_z_lcd[arrIdx] & 0x1000) {
      mAxisBuf.tmp_data_z_lcd[arrIdx] = 0x0FFF;
    }
  } else {
    if(!(mAxisBuf.tmp_data_z_lcd[arrIdx] & 0x1000)) {
      mAxisBuf.tmp_data_z_lcd[arrIdx] = 0x3001;
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

void ProcessTextStream(uint8_t which, uint8_t *string, int index) {
  char *TokenOne = NULL;
  char *TokenTwo = NULL;
  int arrIdx = index;
  
  switch(which) {
  case EQ_ONE :
    //TokenOne = strtok(string, ","); // result contains Date. ex) 20130428
    //DAQBoardOne[arrIdx].Date = TokenOne;
    //TokenOne = strtok(NULL, ","); // result contains Time. ex) 21384729
    //DAQBoardOne[arrIdx].Time = TokenOne;
    TokenOne = strtok(string, ","); // result contains Axis X Data. ex) +4096
    DAQBoardOne[arrIdx].AxisX = TokenOne;
    TokenOne = strtok(NULL, ","); // result contains Axis Y Data. ex) +4096
    DAQBoardOne[arrIdx].AxisY = TokenOne;
    TokenOne = strtok(NULL, ","); // result contains Axis Z Data. ex) +4096
    DAQBoardOne[arrIdx].AxisZ = TokenOne;
    break;
    
  case EQ_TWO :
    //TokenTwo = strtok(string, ","); // result contains Date. ex) 20130428
    //DAQBoardTwo[arrIdx].Date = TokenTwo;
    //TokenTwo = strtok(NULL, ","); // result contains Time. ex) 21384729
    //DAQBoardTwo[arrIdx].Time = TokenTwo;
    TokenTwo = strtok(string, ","); // result contains Axis X Data. ex) +4096
    DAQBoardTwo[arrIdx].AxisX = TokenTwo;
    TokenTwo = strtok(NULL, ","); // result contains Axis Y Data. ex) +4096
    DAQBoardTwo[arrIdx].AxisY = TokenTwo;
    TokenTwo = strtok(NULL, ","); // result contains Axis Z Data. ex) +4096
    DAQBoardTwo[arrIdx].AxisZ = TokenTwo;
    break;
  case PC_Cli: 
    break;
  default: break;
  }
}

void CheckSignAndToInt(int index) {
  // for code efficiency
  uint8_t arrIdx = index;
  
  /* Axis X --------------------------------------*/
  //when it is positive value
  if(!(mAxisBuf.tmp_data_x_lcd[arrIdx] & 0x2000)) {
    // no extra work needed when it is positive
  } else { //when it is negative value
    mAxisBuf.tmp_data_x_lcd[arrIdx] = (~mAxisBuf.tmp_data_x_lcd[arrIdx] + 1) & 0xFFF;
    mAxisBuf.tmp_data_x_lcd[arrIdx] |= 0xFFFFF000;  // set MSB of int type
  }
  
  /* Axis Y --------------------------------------*/
  if(!(mAxisBuf.tmp_data_y_lcd[arrIdx] & 0x2000)) {
    // no extra work needed when it is positive
  } else {
    mAxisBuf.tmp_data_y_lcd[arrIdx] = (~mAxisBuf.tmp_data_y_lcd[arrIdx] + 1) & 0xFFF;
    mAxisBuf.tmp_data_y_lcd[arrIdx] |= 0xFFFFF000;  // set MSB of int type
  }
  
  /* Axis Z --------------------------------------*/
  if(!(mAxisBuf.tmp_data_z_lcd[arrIdx] & 0x2000)) {
    // no extra work needed when it is positive
  } else {
    mAxisBuf.tmp_data_z_lcd[arrIdx] = (~mAxisBuf.tmp_data_z_lcd[arrIdx] + 1) & 0xFFF;
    mAxisBuf.tmp_data_z_lcd[arrIdx] |= 0xFFFFF000;  // set MSB of int type
  }   
}

