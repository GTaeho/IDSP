/*
 * Calendar.c
 *
 *  Created on: 2013. 6. 1.
 *      Author: Park Jin Hyun
 */


/* Includes ------------------------------------------------------------------*/
#include "calendar.h"


/* Private functions ---------------------------------------------------------*/
char Cal_Buf[8] = { 2, 0, 1, 3, 0, 6, 1, 4 };	// Y, Y, Y, Y, M, M, D, D

int year_tmp;

/* functions */

void IncreaseSingleYear(void) {
  Cal_Buf[3]++;
  if (Cal_Buf[3] == 10) {
    Cal_Buf[2]++;
    Cal_Buf[3] = 0;
    if (Cal_Buf[2] == 10) {
      Cal_Buf[2] = 0;
      Cal_Buf[1]++;
      if (Cal_Buf[1] == 10) {
        Cal_Buf[1] = 0;
        Cal_Buf[0]++;
      }
    }
  }
}

void IncreaseSingleDay(void) {
  Cal_Buf[7]++;            //1일씩 증가

  if (((year_tmp % 4 == 0) && (year_tmp % 100 != 0)) || (year_tmp % 400 == 0)) {
    if ((Cal_Buf[4] == 0) && (Cal_Buf[5] == 2)) {
      if (Cal_Buf[7] == 10) {
        Cal_Buf[6]++;
        Cal_Buf[7] = 0;
      }

      if ((Cal_Buf[6] == 3) && (Cal_Buf[7] == 0)) {     //29일인지 비교
        Cal_Buf[5]++;                 //월 증가
        Cal_Buf[6] = 0;              //일의 10의자리를 0으로
        Cal_Buf[7] = 1;                //일의 1의자리를 1로
      }
    }
    
  } else {
    
    if ((Cal_Buf[4] == 0) && (Cal_Buf[5] == 2)) {
      if (Cal_Buf[7] == 10) {                     //일이 10이면       
        Cal_Buf[6]++;                        //일의 10의자리 증가
        Cal_Buf[7] = 0;                        //1의 1의자리는 0으로 초기화
      }

      if ((Cal_Buf[6] == 2) && (Cal_Buf[7] == 9)) {
        Cal_Buf[5]++;                //월 증가
        Cal_Buf[6] = 0;              //일의 10의자리0으로
        Cal_Buf[7] = 1;                //일의 1의자리 1로 초기화
      }
    }
  }

  if ((Cal_Buf[5] == 1) || (Cal_Buf[5] == 3) || (Cal_Buf[5] == 5)
            || (Cal_Buf[5] == 7) || (Cal_Buf[5] == 8)
            || ((Cal_Buf[4] == 1) && (Cal_Buf[5] == 0))
            || ((Cal_Buf[4] == 1) && (Cal_Buf[5] == 2))) {

    if ((Cal_Buf[6] == 3) && (Cal_Buf[7] == 2)) {
      Cal_Buf[5]++;
      Cal_Buf[6] = 0;              //십의자리 일은0으로
      Cal_Buf[7] = 1;              //일은 1로지만 밑에서 1로증가시키기때문에 0으로

      if (Cal_Buf[5] == 10) {
              Cal_Buf[4]++;
              Cal_Buf[5] = 0;
      }
    }

    if (Cal_Buf[7] == 10) {
      Cal_Buf[6]++;
      Cal_Buf[7] = 0;
    }

    if ((Cal_Buf[4] == 1) && (Cal_Buf[5] == 3)) {
      IncreaseSingleYear();                            //년도증가 함수
      Cal_Buf[4] = 0;                //년도증가시 01월 01일로 초기화
      Cal_Buf[5] = 1;
      Cal_Buf[6] = 0;
      Cal_Buf[7] = 1;
    }
  }

  if ((Cal_Buf[5] == 4) || (Cal_Buf[5] == 6) || (Cal_Buf[5] == 9)
            || ((Cal_Buf[4] == 1) && (Cal_Buf[5] == 1))) {
    if ((Cal_Buf[4] == 1) && (Cal_Buf[5] == 3)) {
      IncreaseSingleYear();	//년도 증가 함수
      Cal_Buf[4] = 0;	//년도증가시 01월 01일로 초기화
      Cal_Buf[5] = 1;
      Cal_Buf[6] = 0;
      Cal_Buf[7] = 1;
    }

    if (Cal_Buf[7] == 10) {
      Cal_Buf[6]++;
      Cal_Buf[7] = 0;
    }

    if ((Cal_Buf[6] == 3) && (Cal_Buf[7] == 1)) {
      Cal_Buf[5]++;
      Cal_Buf[6] = 0;	//십의자리 일은0으로
      Cal_Buf[7] = 1;

      if (Cal_Buf[5] == 10) {
              Cal_Buf[4]++;
              Cal_Buf[5] = 0;
      }
    }
  }
}

void TranslateIntoYear(int year) {
  int data;
  data = year;
  
  Cal_Buf[0] = (data / 1000) % 10;
  Cal_Buf[1] = (data / 100) % 10;
  Cal_Buf[2] = (data / 10) % 10;
  Cal_Buf[3] = (data / 1) % 10;
}

void TranslateIntoMonth(int month) {
  int data;
  data = month;
  
  Cal_Buf[4] = (data / 10) % 10;
  Cal_Buf[5] = (data / 1) % 10;
}

void TranslateIntoDay(int day) {
  int data;
  data = day;
  
  Cal_Buf[6] = (data / 10) % 10;
  Cal_Buf[7] = (data / 1) % 10;
}

int GetYearAndMergeToInt(void) {
  int result;
  result = (Cal_Buf[0] * 1000) + (Cal_Buf[1] * 100) + (Cal_Buf[2] * 10)
                  + (Cal_Buf[3] * 1);
  return result;
}

int GetMonthAndMergeToInt(void) {
  int result;
  result = (Cal_Buf[4] * 10) + (Cal_Buf[5] * 1);
  return result;
}

int GetDayAndMergeToInt(void) {
  int result;
  result = (Cal_Buf[6] * 10) + (Cal_Buf[7] * 1);
  return result;
}