/*
 * main.c
 *
 *  Created on: 2013. 5. 30.
 *      Author: Larry G
 */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
typedef enum {
	false, true
} bool;

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern short EQ_SAMPLE[];

int Qt, Qf, ATFCn, THref, THn;
int SumQf, SumATFCn;
int OldestArrayValue;
int OldestSumQfValue, NewestSumQfValue;
int THref, THn, THnpo, dTH;
int PreTRG, TRG;
bool isThisFirstBoot = true;
bool FlagPreTRG;
int TrgOffLevel;
int PickingTime;
bool EventDetection, REC;

int Smp_Buf[50];
int SampleCount;
int RefreshCount;
int pData;	// previous data

int L = 50; // sample
int T = 1000; // sample
int RefreshTime = 300;  // sample
int N = 6;  // sample
int M = 50; // sample

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

int ShiftArrayAndAddData(int *array, int newData) {
	int i;
	int result;

	// shift data to older address
	for (i = 0; i < 50; i++) {
		if (i == 49) {
			array[i] = newData;	// insert new data to the newest address
		} else if (i == 0) {
			result = array[i];
			array[i] = array[i + 1];
		} else {
			array[i] = array[i + 1];
		}
	}
	return result;
}

void ATFCAlgorithm(int mData) {
	int data = mData;

	//ATFC:
	/* Calculation of ATFC */
	if (SampleCount == 0) {
		Qt = abs(data);
		SumQf = abs(0 - abs(data));
		pData = data;	// fill in previous data for the first time
		Smp_Buf[SampleCount] = mData;	// fill in first data to the newest address of the array
	} else if (SampleCount < L - 1) {	// SampleCount < 49
		Qt += abs(data);
		SumQf += abs(abs(pData) - abs(data));	// subtract current data from previous data
		pData = data; // fill in previous data
		Smp_Buf[SampleCount] = data; // fill in incoming data to the newest address of the array
	} else if (SampleCount == L - 1) {	// SampleCount = 49
		Qt += abs(data);
		SumQf += abs(abs(pData) - abs(data));
		Smp_Buf[SampleCount] = data;// fill in last 50th data to the newest address of the array

		Qf = 50 * SumQf;
		ATFCn = Qt + Qf;

		/*
		 printf("\rSampleCount = %d, ", SampleCount);
		 printf("\tQt = %d, ", Qt);
		 printf("\tSumQf = %d, ", SumQf);
		 printf("\tQf = %d, ", Qf);
		 printf("\tATFCn = %d", ATFCn);
		 */

	} else {  // SampleCount >= 50
		Qt = 0; // reset Qt

		if (SampleCount == 50) {
			OldestSumQfValue = abs(0 - abs(Smp_Buf[0]));
			NewestSumQfValue = abs(abs(Smp_Buf[49]) - abs(data));
		} else {
			OldestSumQfValue = abs(abs(OldestArrayValue) - abs(Smp_Buf[0]));
		}

		OldestArrayValue = ShiftArrayAndAddData(Smp_Buf, data);
		int arrIdx;
		for (arrIdx = 0; arrIdx < L; arrIdx++) {
			Qt += abs(Smp_Buf[arrIdx]);
		}
		NewestSumQfValue = abs(abs(Smp_Buf[48]) - abs(Smp_Buf[49]));
		SumQf = SumQf - OldestSumQfValue + NewestSumQfValue;

		Qf = 50 * SumQf;
		ATFCn = Qt + Qf;

		/*
		 if (SampleCount < 52) {
		 printf("\rSampleCount = %d, ", SampleCount);
		 printf("\tQt = %d, ", Qt);
		 printf("\tSumQf = %d, ", SumQf);
		 printf("\tQf = %d, ", Qf);
		 printf("\tATFCn = %d", ATFCn);
		 }
		 */

		if (SampleCount > 4238 && SampleCount < 4244) {
			printf("\rSampleCount = %d,", SampleCount);
			printf("\tQt = %d,", Qt);
			printf("\tSumQf = %d,", SumQf);
			printf("\tQf = %d,", Qf);
			printf("\tATFCn = %d", ATFCn);
		}

		if (isThisFirstBoot) {
			goto PARAMINIT;
		} else {
			goto CHECKREFRESHTIME;
		}

		/* Parameter Initialization */
		PARAMINIT: if (SampleCount < T + 50 && SampleCount >= L) { // SampleCount < 1050 && SampleCount >= 50
			SumATFCn += ATFCn;

			if (SampleCount == T - 1) {
				printf("\rSampleCount = %d,", SampleCount);
				printf("\tQt = %d,", Qt);
				printf("\tSumQf = %d,", SumQf);
				printf("\tQf = %d,", Qf);
				printf("\tATFCn = %d", ATFCn);
				printf("\tSumATFCn = %d", SumATFCn);
			}

			if (SampleCount == T + 49) {		// SampleCount = 1049
				SumATFCn += ATFCn;
				THref = SumATFCn >> 9; // instead of multiply of (2/T), divide 512 by shift 9 times
				THn = THref;

				isThisFirstBoot = false;

				printf("\rSampleCount = %d,", SampleCount);
				printf("\tQt = %d,", Qt);
				printf("\tSumQf = %d,", SumQf);
				printf("\tQf = %d,", Qf);
				printf("\tATFCn = %d", ATFCn);
				printf("\tSumATFCn = %d,", SumATFCn);
				printf("\tTHref = %d,", THref);
				printf("\tTHn = %d", THn);
			}
		}

		CHECKREFRESHTIME: if (SampleCount) {
			if (RefreshCount == RefreshTime - 1) {	// RefreshTime = 299
				RefreshTime = 0;

				//goto REFERENCETHRESHOLDVALUECONTROL;
				printf("\r goto REFERENCETHRESHOLDVALUECONTROL;");
			}
			RefreshCount++;

		} else {
			goto INSTANTANEOUSTHRESHOLDVALUECONTROL;
		}

		/* Reference Threshold Value Control */
		/*
		 REFERENCETHRESHOLDVALUECONTROL: if (SampleCount < T - 1) {
		 SumATFCn += ATFCn;
		 } else if (SampleCount == T - 1) {
		 THref = SumATFCn >> 9; // instead of multiply of (2/T), divide 512 by shift 9 times
		 //goto INSTANTANEOUSTHRESHOLDVALUECONTROL;
		 }
		 */

		/* Instantaneous Threshold Value Control */
		INSTANTANEOUSTHRESHOLDVALUECONTROL: if (SampleCount > T + 49) {
			dTH = ATFCn - THn;
			THnpo = THn + dTH;

			if (!(SampleCount % 100)) {
				printf("\rSampleCount = %d, ", SampleCount);
				printf("\tATFCn = %d, ", ATFCn);
				printf("\tTHn = %d, ", THn);
				printf("\tdTH = %d, ", dTH);
				printf("\tTHnpo = %d,", THnpo);
			}

			if (ATFCn >= THn) {
				PreTRG++;
				printf("\tPreTRG = %d,", PreTRG);
			} else {
				PreTRG = 0;
				printf("\tPreTRG = %d,", PreTRG);
			}

			if (ATFCn >= THref) {
				TRG++;
				printf("\tTRG = %d,", TRG);

			} else {
				TRG = 0;
				printf("\tTRG = %d,", TRG);
			}

			if ((PreTRG >= N) && (TRG > 0)) {
				FlagPreTRG = true;
				printf("\tFlagPreTRG = true, ");
			} else {
				FlagPreTRG = false;
				printf("\tFlagPreTRG = false, ");
			}

			if ((FlagPreTRG == true) && (TRG >= M)) {
				/* Compensation of Picking Time */
				TrgOffLevel = THref >> 1; // gamma * THref = 0.5 * THref = THref / 2 = THref >> 1

				/* Event Detection */
				EventDetection = true;

				RECORDING:
				/* Data Recording */
				REC = true;

				if (ATFCn <= TrgOffLevel) {
					REC = false;
					printf("\tREC = false, goto ATFC");
					//goto ATFC;
				} else {
					goto RECORDING;
				}

			} else {
				printf("\t goto ATFC");
				//goto ATFC;
			}
		}
	}
	// increase sample count each time this function executes
	SampleCount++;
}

/**
 * @brief  Main program
 * @param  None
 * @retval None
 */
int main(void) {
	/*!< At this stage the microcontroller clock setting is already configured,
	 this is done through SystemInit() function which is called from startup
	 file (startup_stm32f10x_xx.s) before to branch to application main.
	 To reconfigure the default setting of SystemInit() function, refer to
	 system_stm32f10x.c file
	 */

	// When everything is set, print message
	printf("- System is ready -\n");
	printf("- Begin ATFC Algorithm -\n");

	int cnt;
	for (cnt = 0; cnt < 5000; cnt++) {
		/* Execute ATFC Algorithm */
		ATFCAlgorithm(EQ_SAMPLE[cnt]);
	}
	return 0;
}

