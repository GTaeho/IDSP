/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
typedef enum{false, true}bool;

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern short EQ_SAMPLE[];

extern volatile uint16_t TimerCount;

volatile int Alpha;
volatile int Beta;
volatile int Gamma;
volatile int L;
volatile int M;
volatile int N;
volatile int T;
volatile int RefreshTime;

int Qt, Qf, ATFCn;
int SumQf, SumATFCn, mSumATFCn;
int OldestArrayValue;
int OldestSumQfValue, NewestSumQfValue;
int THref, THn, THnpo, dTH;
int AvgTH;
int PreTRG, TRG;
int TrgOffLevel;
int PickingTime;

bool REC;
bool isDetected;
bool isThisFirstBoot = true;
bool FlagPreTRG;
volatile bool EventDetection;
volatile bool ParamCompleteFlag;

int Smp_Buf[50];
int InstSumATFC[10];
int SampleCount;
int ParamCount;
int RefreshCount;
int pData;	// previous data
int InstSumCount;
int SampleIndex;
int mIdx;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

void ATFCAlgorithmParameterSetup(void) {
  if(BKP_ReadBackupRegister(BKP_DR9) == 0x0000) {
    printf("\r\n\r\n============= ATFC Parameter List ===============\r\n");
    printf(" * No Backup data found. Use default values instead\r\n");
    printf(" * Parameters are as follows :\r\n");
    
    int mAlpha, mBeta, mL, mM, mN, mT, mRefreshTime;
    int mGamma;
    
    Alpha = 100;
    Beta = 1;
    Gamma = 5;
    L = 50; // sample
    M = 50; // sample
    N = 5;  // sample
    T = 200; // sample
    RefreshTime = 100;  // sample
    
    mAlpha = Alpha; mBeta = Beta; mGamma = Gamma;
    mL = L; mM = M; mN = N; mT = T; mRefreshTime = RefreshTime;
    
    printf("Alpha = %d, Beta = %d, Gamma = %d * 10^-1\r\n"
           "L = %d, M = %d, N = %d\r\n"
           "T = %d, RefreshTime = %d\r\n",
              mAlpha, mBeta, mGamma, mL, mM, mN, mT, mRefreshTime);
  } else {
    
    printf("\r\n\r\n============= ATFC Parameter List ===============\r\n");
    printf(" * Found parameter data! Use those backup values\r\n");
    printf(" * Modified Parameters are as follows :\r\n");
    
    int mAlpha, mBeta, mL, mM, mN, mT, mRefreshTime;
    int mGamma;
    
    Alpha = BKP_ReadBackupRegister(BKP_DR9);
    Beta = BKP_ReadBackupRegister(BKP_DR10);
    Gamma = BKP_ReadBackupRegister(BKP_DR11);
    L = BKP_ReadBackupRegister(BKP_DR12); // sample
    M = BKP_ReadBackupRegister(BKP_DR13); // sample
    N = BKP_ReadBackupRegister(BKP_DR14); // sample
    T = BKP_ReadBackupRegister(BKP_DR15); // sample
    RefreshTime = BKP_ReadBackupRegister(BKP_DR16);  // sample
    
    mAlpha = Alpha; mBeta = Beta; mGamma = Gamma;
    mL = L; mM = M; mN = N; mT = T; mRefreshTime = RefreshTime;
    
    printf(" Alpha = %d, Beta = %d, Gamma = %d * 10^-1\r\n"
           "  L = %d, M = %d, N = %d\r\n"
           "  T = %d, RefreshTime = %d\r\n",
              mAlpha, mBeta, mGamma, mL, mM, mN, mT, mRefreshTime);
  }
}

int ShiftArrayAndAddData(int *array, int newData, int mArrSize) {
  int i;
  int arrSize;
  int result;
  
  arrSize = mArrSize;

  // shift data to older address
  for (i = 0; i < arrSize; i++) {
    if (i == 0) {
      result = array[i];
      array[i] = array[i+1];
    } else if (i != 0 && i < arrSize-1) {
      array[i] = array[i+1];
    } else if(i == arrSize-1) {
      array[i] = newData;   // insert new data to the newest address
    }
  }
  return result;
}

/* ATFC Algorithm */
void ATFCAlgorithm(int mData) {
  int data = mData;
  
  if(data & 0x2000) {
    data = (~data+1) & 0xFFF;
  } else {
    data &= 0xFFF;
  }

  /* Calculation of ATFC */
  if (SampleCount == 0) {
    Qt = abs(data);
    SumQf = abs(0 - abs(data));
    pData = data; // fill in previous data for the first time
    Smp_Buf[SampleCount] = data; // fill in first data to the newest address of the array
  } else if (SampleCount < L - 1) {	// SampleCount < 49
    Qt += abs(data);
    SumQf += abs(abs(pData) - abs(data));	// subtract current data from previous data
    pData = data; // fill in previous data
    Smp_Buf[SampleCount] = data; // fill in incoming data to the newest address of the array
  } else if (SampleCount == L - 1) {	// SampleCount = L - 1
    Qt += abs(data);
    SumQf += abs(abs(pData) - abs(data));
    Smp_Buf[SampleCount] = data;// fill in last 50th data to the newest address of the array

    Qf = Alpha * Qt;
    ATFCn = Qt + Qf;

    goto PARAMINIT;  // very first value is needed for calculate SumATFCn

    /*
     printf("\rSampleCount = %f, ", SampleCount);
     printf("\tQt = %f, ", Qt);
     printf("\tSumQf = %f, ", SumQf);
     printf("\tQf = %f, ", Qf);
     printf("\tATFCn = %f", ATFCn);
     */

  } else {  // SampleCount >= L
    Qt = 0;

    if (SampleCount == L) {
      OldestSumQfValue = abs(0 - abs(Smp_Buf[0]));
      NewestSumQfValue = abs(abs(Smp_Buf[49]) - abs(data));
    } else {
      OldestSumQfValue = abs(abs(OldestArrayValue) - abs(Smp_Buf[0]));
    }

    OldestArrayValue = ShiftArrayAndAddData(Smp_Buf, data, sizeof(Smp_Buf) / sizeof(*Smp_Buf));
    int arrIdx;
    for (arrIdx = 0; arrIdx < L; arrIdx++) {
    	Qt += abs(Smp_Buf[arrIdx]);
    }
    NewestSumQfValue = abs(abs(Smp_Buf[48]) - abs(Smp_Buf[49]));
    SumQf = SumQf - OldestSumQfValue + NewestSumQfValue;

    Qf = Alpha * SumQf;
    ATFCn = Qt + Qf;

    if (isDetected) {
      goto DATARECORDING;
    }

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
      switch (ParamCount) {
      case 99:
      case 199:
      case 299:
      case 399:
      case 499:
      case 599:
      case 699:
      case 799:
      case 899:
        InstSumATFC[InstSumCount] = mSumATFCn;
        mSumATFCn = 0;	// reset mSumATFCn to restart add sequence
        InstSumCount++;
        ParamCount++;
        goto INCREASECOUNT;
        break;
      case 999:
        InstSumATFC[InstSumCount] = mSumATFCn;
        mSumATFCn = 0;	// reset mSumATFCn to restart add sequence
        ParamCount = 0;
        int i;
        for (i = 0; i < 10; i++) {
          SumATFCn += InstSumATFC[i];
        }
        
        AvgTH = 1000;
        THref = (2 * SumATFCn) / AvgTH;
        THn = THref;
        
        isThisFirstBoot = false;
  
        /*
        printf("\r\n\nSampleCount = %d, ", SampleCount);
        printf("Qt = %d, ", Qt);
        printf("Qf = %d, ", Qf);
        printf("ATFCn = %d, ", ATFCn);
        printf("SumATFCn = %d, ", SumATFCn);
        printf("THn = %1.4f, ", THn);
        printf("dTH = %1.0f, ", dTH);
        printf("THref = %1.4f ", THref);
        */
  
        ParamCompleteFlag = true;
        RefreshCount = 0;	// set this once before branch into check refresh time routine
        goto CHECKREFRESHTIME;
        break;
  
      default:
        ParamCount++;
        goto INCREASECOUNT;
        break;
      }

    /* Check Refresh Time */
    CHECKREFRESHTIME: // SampleCount depend on parameter setting
      mSumATFCn += ATFCn;	// constantly add ATFC in order to sum up and shift array
      if(RefreshTime == 100) {
        switch (RefreshCount) {
        case 99:
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
          ShiftArrayAndAddData(InstSumATFC, mSumATFCn, sizeof(InstSumATFC) / sizeof(*InstSumATFC));
          mSumATFCn = 0;	// reset mSumATFCn to restart add sequence
          goto REFERENCETHRESHOLDVALUECONTROL;
          break;
        default:
          RefreshCount++;
          goto INSTANTANEOUSTHRESHOLDVALUECONTROL;
          break;
        }
      } else if(RefreshTime == 200) {
        switch (RefreshCount) {
        case 99:
          ShiftArrayAndAddData(InstSumATFC, mSumATFCn, sizeof(InstSumATFC) / sizeof(*InstSumATFC));
          mSumATFCn = 0;  // reset mSumATFCn to restart add sequence
          RefreshCount++;
          goto INSTANTANEOUSTHRESHOLDVALUECONTROL;
          break;
        case 199:
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
          ShiftArrayAndAddData(InstSumATFC, mSumATFCn, sizeof(InstSumATFC) / sizeof(*InstSumATFC));
          mSumATFCn = 0;  // reset mSumATFCn to restart add sequence
          goto REFERENCETHRESHOLDVALUECONTROL;
          break;
        default:
          RefreshCount++;
          goto INSTANTANEOUSTHRESHOLDVALUECONTROL;
          break;
        }
      } else if(RefreshTime == 300) {
        switch (RefreshCount) {
        case 99:
        case 199:
          ShiftArrayAndAddData(InstSumATFC, mSumATFCn, sizeof(InstSumATFC) / sizeof(*InstSumATFC));
          mSumATFCn = 0;	// reset mSumATFCn to restart add sequence
          RefreshCount++;
          goto INSTANTANEOUSTHRESHOLDVALUECONTROL;
          break;
        case 299: // RefreshTime - 1
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
          ShiftArrayAndAddData(InstSumATFC, mSumATFCn, sizeof(InstSumATFC) / sizeof(*InstSumATFC));
          mSumATFCn = 0;	// reset mSumATFCn to restart add sequence
          goto REFERENCETHRESHOLDVALUECONTROL;
          break;
        default:
          RefreshCount++;
          goto INSTANTANEOUSTHRESHOLDVALUECONTROL;
          break;
        }
      } else if(RefreshTime == 500) {
        switch (RefreshCount) {
        case 99:
        case 199:
        case 299:
        case 399:
          ShiftArrayAndAddData(InstSumATFC, mSumATFCn, sizeof(InstSumATFC) / sizeof(*InstSumATFC));
          mSumATFCn = 0;  // reset mSumATFCn to restart add sequence
          RefreshCount++;
          goto INSTANTANEOUSTHRESHOLDVALUECONTROL;
          break;
        case 499:	// RefreshTime - 1
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
          ShiftArrayAndAddData(InstSumATFC, mSumATFCn, sizeof(InstSumATFC) / sizeof(*InstSumATFC));
          mSumATFCn = 0;	// reset mSumATFCn to restart add sequence
          goto REFERENCETHRESHOLDVALUECONTROL;
          break;
        default:
          RefreshCount++;
          goto INSTANTANEOUSTHRESHOLDVALUECONTROL;
          break;
        }
      } else if(RefreshTime == 1000) {
        switch (RefreshCount) {
        case 99:
        case 199:
        case 299:
        case 399:
        case 499:
        case 599:
        case 699:
        case 799:
        case 899:
          ShiftArrayAndAddData(InstSumATFC, mSumATFCn, sizeof(InstSumATFC) / sizeof(*InstSumATFC));
          mSumATFCn = 0;	// reset mSumATFCn to restart add sequence
          RefreshCount++;
          goto INSTANTANEOUSTHRESHOLDVALUECONTROL;
          break;
        case 999: // RefreshTime - 1
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
          ShiftArrayAndAddData(InstSumATFC, mSumATFCn, sizeof(InstSumATFC) / sizeof(*InstSumATFC));
          mSumATFCn = 0;	// reset mSumATFCn to restart add sequence
          goto REFERENCETHRESHOLDVALUECONTROL;
          break;
        default:
          RefreshCount++;
          goto INSTANTANEOUSTHRESHOLDVALUECONTROL;
          break;
        }
      }

    /* Reference Threshold Value Control */
    REFERENCETHRESHOLDVALUECONTROL: 
      RefreshCount = 0;
      SumATFCn = 0;
      
      if(T == 100) {
          SumATFCn = InstSumATFC[9];
          
          AvgTH = 100;
          THref = (2 * SumATFCn) / AvgTH;
          
      } else if( T == 200) {
          SumATFCn = InstSumATFC[9];
          SumATFCn += InstSumATFC[8];
          
          AvgTH = 200;
          THref = (2 * SumATFCn) / AvgTH;
      } else if( T == 300) {
        for (mIdx = 9; mIdx > 6; mIdx--) {
          SumATFCn += InstSumATFC[mIdx];
        }
        AvgTH = 300;
        THref = (2 * SumATFCn) / AvgTH;
      } else if( T == 500) {
        for (mIdx =9 ; mIdx > 4; mIdx--) {
          SumATFCn += InstSumATFC[mIdx];
        }
        AvgTH = 500;
        THref = (2 * SumATFCn) / AvgTH;
      } else if( T == 1000) {
        for (mIdx = 9; mIdx >= 0; mIdx--) {
          SumATFCn += InstSumATFC[mIdx];
        }
        AvgTH = 1000;
        THref = (2 * SumATFCn) / AvgTH;
      }

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

    /* Instantaneous Threshold Value Control */
    INSTANTANEOUSTHRESHOLDVALUECONTROL: 
      dTH = Beta * (ATFCn - THn); // Beta = 1
      THnpo = THn + dTH;

      /*
      if (SampleCount == 4241 || SampleCount == 4243) {
        printf("\r\n\nSampleCount = %d,", SampleCount);
        printf("Qt = %d,", Qt);
        printf("Qf = %d,", Qf);
        printf("ATFCn = %d,", ATFCn);
        printf("SumATFCn = %d,", SumATFCn);
        printf("THn = %1.0f,", THn);
        printf("dTH = %1.0f,", dTH);
        printf("THref = %1.4f", THref);
        printf("TrgOffLevel = %1.4f", TrgOffLevel);
      }
      */
  
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
  
      if ((FlagPreTRG == true) && (TRG >= M)) {
        /* Compensation of Picking Time */
        TrgOffLevel = (Gamma * THref) / 10;
        
        /* Event Detection */
        EventDetection = true;
        
        DATARECORDING :
        /* Data Recording */
        REC = true;
        
        /*
        if (SampleCount < 4500 || SampleCount > 14000) {
          printf("\r\n\nSampleCount = %d,", SampleCount);
          printf("Qt = %d,", Qt);
          printf("Qf = %d,", Qf);
          printf("ATFCn = %d,", ATFCn);
          printf("THn = %1.4f,", THn);
          printf("dTH = %1.0f,", dTH);
          printf("THref = %1.4f,", THref);
          printf("TrgOffLevel = %1.4f", TrgOffLevel);
          printf("EventDetection!"); 
        }
        */
  
        if (ATFCn <= TrgOffLevel) {
          REC = false;
          isDetected = false;
          EventDetection = false;
          goto INCREASECOUNT;
        } else {
          isDetected = true;
          goto INCREASECOUNT;
        }
      } else {
        goto INCREASECOUNT;
      }
    }

  // increase sample count each time this function executes
  INCREASECOUNT: // if SampleCount reaches 0xFFFF go back to L
    if(SampleCount >= 0xFFFF) SampleCount = L;
    else SampleCount++;
}

