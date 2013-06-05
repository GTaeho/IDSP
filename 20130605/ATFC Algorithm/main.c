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

int Qt, Qf, ATFCn;
int SumQf, SumATFCn, mSumATFCn;
int OldestArrayValue;
int OldestSumQfValue, NewestSumQfValue;
float THref, THn, THnpo, dTH;
float AvgTH = (float) 2 / (float) 1000;
int PreTRG, TRG;
bool isThisFirstBoot = true;
bool FlagPreTRG;
float TrgOffLevel;
int PickingTime;
bool EventDetection, REC;

int Smp_Buf[50];
int InstSumATFC[10];
int SampleCount;
int RefreshCount;
int pData;	// previous data
int InstSumCount;

int Alpha = 100;
int Beta = 1;
float Gamma = 0.3;
int L = 50; // sample
int T = 1000; // sample
int RefreshTime = 300;  // sample
int N = 6;  // sample
int M = 50; // sample

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

int ShiftArrayAndAddData(int *array, int newData, int mArrSize) {
	int i;
	int arrSize = mArrSize;
	int result;

	// shift data to older address
	for (i = 0; i < arrSize; i++) {
		if (i == 0) {
			result = array[i];
			array[i] = array[i + 1];
		} else if (i == arrSize - 1) {
			array[i] = newData;	// insert new data to the newest address
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

		Qf = Alpha * SumQf;
		ATFCn = Qt + Qf;

		/*
		 printf("\rSampleCount = %f, ", SampleCount);
		 printf("\tQt = %f, ", Qt);
		 printf("\tSumQf = %f, ", SumQf);
		 printf("\tQf = %f, ", Qf);
		 printf("\tATFCn = %f", ATFCn);
		 */

	} else {  // SampleCount >= 50
		Qt = 0; // reset Qt

		if (SampleCount == 50) {
			OldestSumQfValue = abs(0 - abs(Smp_Buf[0]));
			NewestSumQfValue = abs(abs(Smp_Buf[49]) - abs(data));
		} else {
			OldestSumQfValue = abs(abs(OldestArrayValue) - abs(Smp_Buf[0]));
		}

		OldestArrayValue = ShiftArrayAndAddData(Smp_Buf, data,
				sizeof(Smp_Buf) / sizeof(*Smp_Buf));
		int arrIdx;
		for (arrIdx = 0; arrIdx < L; arrIdx++) {
			Qt += abs(Smp_Buf[arrIdx]);
		}
		NewestSumQfValue = abs(abs(Smp_Buf[48]) - abs(Smp_Buf[49]));
		SumQf = SumQf - OldestSumQfValue + NewestSumQfValue;

		Qf = Alpha * SumQf;
		ATFCn = Qt + Qf;

		/*
		 if (SampleCount < 52) {
		 printf("\rSampleCount = %f, ", SampleCount);
		 printf("\tQt = %f, ", Qt);
		 printf("\tSumQf = %f, ", SumQf);
		 printf("\tQf = %f, ", Qf);
		 printf("\tATFCn = %f", ATFCn);
		 }
		 */

		/*
		 if (SampleCount > 4238 && SampleCount < 4244) {
		 printf("\rSampleCount = %f,", SampleCount);
		 printf("\tQt = %f,", Qt);
		 printf("\tSumQf = %f,", SumQf);
		 printf("\tQf = %f,", Qf);
		 printf("\tATFCn = %f", ATFCn);
		 }
		 */

		if (isThisFirstBoot) {
			goto PARAMINIT;
		} else {
			goto CHECKREFRESHTIME;
		}

		/* Parameter Initialization */
		PARAMINIT: mSumATFCn += ATFCn;
		switch (SampleCount) {
		case 148:
		case 248:
		case 348:
		case 448:
		case 548:
		case 648:
		case 748:
		case 848:
		case 948:
			InstSumATFC[InstSumCount] = mSumATFCn;
			mSumATFCn = 0;	// reset mSumATFCn to restart add sequence
			InstSumCount++;
			goto INCREASECOUNT;
			break;
		case 1048:
			InstSumATFC[InstSumCount] = mSumATFCn;
			mSumATFCn = 0;	// reset mSumATFCn to restart add sequence
			int i;
			for (i = 0; i < 10; i++) {
				SumATFCn += InstSumATFC[i];
			}

			THref = AvgTH * (float) SumATFCn;
			THn = THref;

			isThisFirstBoot = false;

			printf("\rSampleCount = %d,", SampleCount);
			printf("\tQt = %d,", Qt);
			printf("\tQf = %d,", Qf);
			printf("\tATFCn = %d", ATFCn);
			printf("\tSumATFCn = %d,", SumATFCn);
			printf("\tTHn = %1.4f,", THn);
			printf("\tdTH = %1.0f,", dTH);
			printf("\tTHref = %1.4f", THref);
			printf("\tInstSumATFC[%d] = %d", InstSumCount, InstSumATFC[InstSumCount]);

			RefreshCount = 0;	// set this once before branch into check refresh time routine
			goto CHECKREFRESHTIME;
			break;

		default:
			goto INCREASECOUNT;
			break;
		}

		/* Check Refresh Time */
		CHECKREFRESHTIME: // SampleCount >= 1048
		mSumATFCn += ATFCn;	// constantly add ATFC in order to sum up and shift array
		switch (RefreshCount) {
		case 99:
		case 199:
			ShiftArrayAndAddData(InstSumATFC, mSumATFCn,
					sizeof(InstSumATFC) / sizeof(*InstSumATFC));
			mSumATFCn = 0;	// reset mSumATFCn to restart add sequence
			RefreshCount++;
			goto INSTANTANEOUSTHRESHOLDVALUECONTROL;
			break;
		case 299:	// RefreshTime - 1
			/*
			 printf("\r * SampleCount = %d,", SampleCount);
			 printf("\tQt = %d", Qt);
			 printf("\tSumQf = %d,", SumQf);
			 printf("\tQf = %d,", Qf);
			 printf("\tATFCn = %d,", ATFCn);
			 printf("\tmSumATFCn = %d,", mSumATFCn);
			 printf("\tTHref = %1.4f,", THref);
			 printf("\tdTH = %1.0f,", dTH);
			 printf("\tTHn = %1.0f,", THn);
			 */
			ShiftArrayAndAddData(InstSumATFC, mSumATFCn,
					sizeof(InstSumATFC) / sizeof(*InstSumATFC));
			mSumATFCn = 0;	// reset mSumATFCn to restart add sequence
			goto REFERENCETHRESHOLDVALUECONTROL;
			break;
		default:
			RefreshCount++;
			goto INSTANTANEOUSTHRESHOLDVALUECONTROL;
			break;
		}

		/* Reference Threshold Value Control */
		REFERENCETHRESHOLDVALUECONTROL: if (RefreshCount == RefreshTime - 1) { // just double check
			RefreshCount = 0;
			SumATFCn = 0;

			int i;
			for (i = 0; i < 10; i++) {
				SumATFCn += InstSumATFC[i];
			}

			THref = AvgTH * (float) SumATFCn;
			/*
			 printf("\rSampleCount = %d,", SampleCount);
			 printf("\tQt = %d,", Qt);
			 printf("\tQf = %d,", Qf);
			 printf("\tATFCn = %d", ATFCn);
			 printf("\tSumATFCn = %d,", SumATFCn);
			 printf("\tTHn = %1.0f,", THn);
			 printf("\tdTH = %1.0f,", dTH);
			 printf("\tTHref = %1.4f", THref);
			 */
			// automatically goes to next routine which is Instantaneous Threshold Value Control
		}

		/* Instantaneous Threshold Value Control */
		INSTANTANEOUSTHRESHOLDVALUECONTROL: // Beta = 1
		dTH = Beta * (ATFCn - THn);
		THnpo = THn + dTH;

		if (ATFCn >= THn) {
			PreTRG++;
		} else {
			PreTRG = 0;
		}
		THn = THnpo;

		if (ATFCn >= THref) {
			TRG++;
		} else {
			TRG = 0;
		}

		if ((PreTRG >= N) && (TRG > 0)) {
			FlagPreTRG = true;
		} else {
			FlagPreTRG = false;
		}

		if (SampleCount == 4241 || SampleCount == 4243) {
			printf("\rSampleCount = %d,", SampleCount);
			printf("\tQt = %d,", Qt);
			printf("\tQf = %d,", Qf);
			printf("\tATFCn = %d", ATFCn);
			printf("\tSumATFCn = %d,", SumATFCn);
			printf("\tTHn = %1.0f,", THn);
			printf("\tdTH = %1.0f,", dTH);
			printf("\tTHref = %1.4f", THref);
			printf("\tTrgOffLevel = %1.4f", TrgOffLevel);
		}

		if ((FlagPreTRG == true) && (TRG >= M)) {
			/* Compensation of Picking Time */
			TrgOffLevel = Gamma * THref;

			/* Event Detection */
			EventDetection = true;

			printf("\rSampleCount = %d,", SampleCount);
			printf("\tQt = %d,", Qt);
			printf("\tQf = %d,", Qf);
			printf("\tATFCn = %d,", ATFCn);
			printf("\tTHn = %1.4f,", THn);
			printf("\tdTH = %1.4f,", dTH);
			printf("\tTHref = %1.4f,", THref);
			printf("\tTrgOffLevel = %1.4f", TrgOffLevel);
			printf("\tEventDetection!");

			//DATARECORDING:
			/* Data Recording */
			REC = true;

			if (ATFCn <= TrgOffLevel) {
				REC = false;
				goto INCREASECOUNT;
			} else {
				//goto DATARECORDING;
			}
		} else {
			goto INCREASECOUNT;
		}
	}

	// increase sample count each time this function executes
	INCREASECOUNT: SampleCount++;
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
	for (cnt = 0; cnt < 20000; cnt++) {
		/* Execute ATFC Algorithm */
		ATFCAlgorithm(EQ_SAMPLE[cnt]);
	}
	return 0;
}

