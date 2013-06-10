/*
 * Calendar.c
 *
 *  Created on: 2013. 6. 1.
 *      Author: Park Jin Hyun
 */


/* Includes ------------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
void IncreaseSingleYear(void);
void IncreaseSingleDay(void);
void TranslateIntoYear(int year);
void TranslateIntoMonth(int month);
void TranslateIntoDay(int day);
int GetYearAndMergeToInt(void);
int GetMonthAndMergeToInt(void);
int GetDayAndMergeToInt(void);