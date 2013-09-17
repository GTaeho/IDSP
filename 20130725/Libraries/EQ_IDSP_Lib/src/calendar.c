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
  Cal_Buf[7]++;            //1�Ͼ� ����

  if (((year_tmp % 4 == 0) && (year_tmp % 100 != 0)) || (year_tmp % 400 == 0)) {
    if ((Cal_Buf[4] == 0) && (Cal_Buf[5] == 2)) {
      if (Cal_Buf[7] == 10) {
        Cal_Buf[6]++;
        Cal_Buf[7] = 0;
      }

      if ((Cal_Buf[6] == 3) && (Cal_Buf[7] == 0)) {     //29������ ��
        Cal_Buf[5]++;                 //�� ����
        Cal_Buf[6] = 0;              //���� 10���ڸ��� 0����
        Cal_Buf[7] = 1;                //���� 1���ڸ��� 1��
      }
    }
    
  } else {
    
    if ((Cal_Buf[4] == 0) && (Cal_Buf[5] == 2)) {
      if (Cal_Buf[7] == 10) {                     //���� 10�̸�       
        Cal_Buf[6]++;                        //���� 10���ڸ� ����
        Cal_Buf[7] = 0;                        //1�� 1���ڸ��� 0���� �ʱ�ȭ
      }

      if ((Cal_Buf[6] == 2) && (Cal_Buf[7] == 9)) {
        Cal_Buf[5]++;                //�� ����
        Cal_Buf[6] = 0;              //���� 10���ڸ�0����
        Cal_Buf[7] = 1;                //���� 1���ڸ� 1�� �ʱ�ȭ
      }
    }
  }

  if ((Cal_Buf[5] == 1) || (Cal_Buf[5] == 3) || (Cal_Buf[5] == 5)
            || (Cal_Buf[5] == 7) || (Cal_Buf[5] == 8)
            || ((Cal_Buf[4] == 1) && (Cal_Buf[5] == 0))
            || ((Cal_Buf[4] == 1) && (Cal_Buf[5] == 2))) {

    if ((Cal_Buf[6] == 3) && (Cal_Buf[7] == 2)) {
      Cal_Buf[5]++;
      Cal_Buf[6] = 0;              //�����ڸ� ����0����
      Cal_Buf[7] = 1;              //���� 1������ �ؿ��� 1��������Ű�⶧���� 0����

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
      IncreaseSingleYear();                            //�⵵���� �Լ�
      Cal_Buf[4] = 0;                //�⵵������ 01�� 01�Ϸ� �ʱ�ȭ
      Cal_Buf[5] = 1;
      Cal_Buf[6] = 0;
      Cal_Buf[7] = 1;
    }
  }

  if ((Cal_Buf[5] == 4) || (Cal_Buf[5] == 6) || (Cal_Buf[5] == 9)
            || ((Cal_Buf[4] == 1) && (Cal_Buf[5] == 1))) {
    if ((Cal_Buf[4] == 1) && (Cal_Buf[5] == 3)) {
      IncreaseSingleYear();	//�⵵ ���� �Լ�
      Cal_Buf[4] = 0;	//�⵵������ 01�� 01�Ϸ� �ʱ�ȭ
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
      Cal_Buf[6] = 0;	//�����ڸ� ����0����
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