/* ###################################################################
 **     Filename    : main.c
 **     Project     : Project
 **     Processor   : MK70FN1M0VMJ12
 **     Version     : Driver 01.01
 **     Compiler    : GNU C Compiler
 **     Date/Time   : 2015-07-20, 13:27, # CodeGen: 0
 **     Abstract    :
 **         Main module.
 **         This module contains user's application code.
 **     Settings    :
 **     Contents    :
 **         No public methods
 **
 ** ###################################################################*/
/*!
 ** @file main.c
 ** @version 6.0
 ** @brief
 **         Main module.
 **         This module contains user's application code.
 */
/*!
 **  @addtogroup main_module main module documentation
 **  @{
 */
/* MODULE main */

#include <Math.h>
// CPU module - contains low level hardware initialization routines
#include "Cpu.h"

// Simple OS
#include "OS.h"

// Analog functions
#include "analog.h"
// UART functions
#include "FIFO.h"
#include "Packet.h"
#include "UART.h"
#include "LEDs.h"
#include "Flash.h"
#include "PIT.h"
#include "RMS.h"
#include "VRR.h"
#include "handle.h"
extern OS_ECB* PITSemaphore;


//BAUD RATE
static const uint32_t BAUD_RATE = 115200;

// ----------------------------------------
// Thread set up
// ----------------------------------------
// Arbitrary thread stack size - big enough for stacking of interrupts and OS use.
#define THREAD_STACK_SIZE 300
#define NB_ANALOG_CHANNELS 3
#define MAX_SAMPLE_SIZE 16


#define SAMPLE_PERIOD 1250000



#define VOLT_PER_BIT 3276.7
#define VOLT(x) 3277*(x)

// Thread stacks
OS_THREAD_STACK(InitModulesThreadStack, THREAD_STACK_SIZE); /*!< The stack for the LED Init thread. */
OS_THREAD_STACK(UART_RecieveStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(UART_TransmitStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(Sample_Stack, THREAD_STACK_SIZE);
OS_THREAD_STACK(SignalsOutput_Stack, THREAD_STACK_SIZE);
OS_THREAD_STACK(HandlePacketStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(RMSThreadStack, THREAD_STACK_SIZE);

static uint32_t AlarmThreadStacks[NB_ANALOG_CHANNELS][THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));

// ----------------------------------------
// Thread priorities
// 0 = highest priority
// ----------------------------------------

static const uint16_t INIT_MODULES_THREAD_PRIORITY = 0;
static const uint16_t UART_RECEIVE_THREAD_PRIORITY = 1;
static const uint16_t UART_TRANSMIT_THREAD_PRIORITY = 7;
static const uint16_t SAMPLE_THREAD_PRIORITY = 2;
static const uint16_t SIGNALOUT_THREAD_PRIORITY = 8;
static const uint16_t HANDLE_PACKET_THREAD_PRIORITY = 9;
static const uint8_t RMS_THREAD_PRIORITY = 3;
const uint8_t ALARM_THREAD_PRIORITIES[NB_ANALOG_CHANNELS] = {4,5,6};


/*! @brief Data structure used to pass Analog configuration to a user thread
 *
 */
typedef struct RMSThreadData
{
  OS_ECB* semaphore;
  uint8_t channelNb;
  uint8_t sampleCount;

} TRMSThreadData;

typedef struct AlarmThreadData
{
  OS_ECB* voltageCheckSemaphore;
  OS_ECB* timeCountSemaphore;
  int16_t sampleArray[MAX_SAMPLE_SIZE];
  uint8_t channelNb;
  int16_t RMS;
  bool voltageAlarm;
  bool outofRange;
  bool raiseRequest;
  bool lowerRequest;
  bool raiseSignal;
  bool lowerSignal;
  uint64_t TimerDelay;
  uint64_t TimerCount;
  //Change to Enum list

} TAlarmThreadData;

/*! @brief Analog thread configuration data
 *
 */

 TAlarmThreadData AlarmThreadData[NB_ANALOG_CHANNELS] =
{
  {
    .voltageCheckSemaphore = NULL,
    .channelNb = 0,
    .RMS = 0,
    .voltageAlarm = false,
    .TimerCount = 0

  },
  {
    .voltageCheckSemaphore = NULL,
    .channelNb = 1,
    .RMS = 0,
    .voltageAlarm = false,
    .TimerCount = 0
  },
  {
    .voltageCheckSemaphore = NULL,
    .channelNb = 2,
    .RMS = 0,
    .voltageAlarm = false,
    .TimerCount = 0
  }
};


OS_ECB* SignalOutputSemaphore;
OS_ECB* PITCountSemaphore;
OS_ECB* RMSCalcSemaphore;

static uint64_t inverseTimerDelay[3];
bool OutofRange = false;

static uint64_t counter = 0;

bool InitSuccess = false;

extern TimerType Mode;
extern uint16union_t * NbRaises = 0;
uint8_t NbRaisesCount = 0;
extern uint16union_t * NbLowers = 0;
uint8_t NbLowersCount = 0;


/*! @brief Initialises modules.
 *
 */
static void InitModulesThread(void* pData)
{

  OS_DisableInterrupts();

  while (!InitSuccess)
  {
    InitSuccess = true;
    InitSuccess &= Packet_Init(BAUD_RATE, CPU_BUS_CLK_HZ);
    InitSuccess &= Flash_Init();
    InitSuccess &= LEDs_Init();
    InitSuccess &= PIT_Init(CPU_BUS_CLK_HZ, NULL, NULL);
    InitSuccess &= Analog_Init(CPU_BUS_CLK_HZ);
  }

  // Generate the global analog semaphores
  for (uint8_t analogNb = 0; analogNb < NB_ANALOG_CHANNELS; analogNb++)
  {
    //check voltage & deviation when out of range voltage detected
    AlarmThreadData[analogNb].voltageCheckSemaphore = OS_SemaphoreCreate(0);
    //ticks the counter
    AlarmThreadData[analogNb].timeCountSemaphore = OS_SemaphoreCreate(0);
  }

  SignalOutputSemaphore = OS_SemaphoreCreate(0);
  //Signals Timer for definite and inverse timing
  PITCountSemaphore     = OS_SemaphoreCreate(0);
  //Signals RMS Calculations
  RMSCalcSemaphore      = OS_SemaphoreCreate(0);

  PIT_Set(SAMPLE_PERIOD, true);
  OS_EnableInterrupts();

  // We only do this once - therefore delete this thread
  OS_ThreadDelete(OS_PRIORITY_SELF);
}

void Sample_Thread(void* pData)
{
  //TODO: Change PIT to 3 channel period
  for (;;)
  {
    static uint8_t sampleCount = 0;
    (void)OS_SemaphoreWait(PITSemaphore,0);

    for (int channelNb =0; channelNb< NB_ANALOG_CHANNELS; channelNb++)
    {
      Analog_Get(channelNb, &(AlarmThreadData[channelNb].sampleArray[sampleCount]));
    }
    sampleCount ++;

    if(sampleCount > 15)
    {
      sampleCount = 0;
      OS_SemaphoreSignal(RMSCalcSemaphore);
    }

    for (int channelNb =0; channelNb < NB_ANALOG_CHANNELS; channelNb++)
    {
      OS_SemaphoreSignal(AlarmThreadData[channelNb].timeCountSemaphore);
    }
  }

}


void RMS_CalcThread (void* pData)
{

  for (;;)
  {
    OS_SemaphoreWait(RMSCalcSemaphore, 0);
    int16_t RMSTest[3];
    int16_t VoltageDeviate[3];

    for (int channelNb = 0; channelNb < NB_ANALOG_CHANNELS; channelNb++)
    {
      RMSTest[channelNb] = RMS_Calculate(AlarmThreadData[channelNb].sampleArray, MAX_SAMPLE_SIZE);
      AlarmThreadData[channelNb].RMS = RMSTest[channelNb];
      VoltageDeviate[channelNb] = VRR_CalcDeviation(RMSTest[channelNb]);
      //Change calculation to nanoseconds if neccesaary
      inverseTimerDelay[channelNb] = (uint64_t)((5E-9*VOLT(0.5))/VoltageDeviate[channelNb]);
    }

    for (int channelNb = 0; channelNb < NB_ANALOG_CHANNELS; channelNb++)
    {
      if (RMSTest[channelNb] < VOLT(2) || RMSTest[channelNb] > VOLT(3))
      {
        AlarmThreadData[channelNb].voltageAlarm = true;
        OS_SemaphoreSignal(SignalOutputSemaphore);
        Analog_Put(2, VOLT(5));
        // signal Functionality not working so added analog_put to show logic
        OS_SemaphoreSignal(AlarmThreadData[channelNb].voltageCheckSemaphore);
      }
      else
      {
        Analog_Put(2, VOLT(0));
        Analog_Put(1, VOLT(0));
        Analog_Put(0, VOLT(0));
        // signal Functionality not working so added analog_put to show logic
        AlarmThreadData[channelNb].voltageAlarm = false;
        AlarmThreadData[channelNb].lowerRequest = false;
        AlarmThreadData[channelNb].raiseRequest = false;
        OS_SemaphoreSignal(SignalOutputSemaphore);
      }
    }

  }
}

void Alarm_Thread (void* pData)
{
  #define alarmData ((TAlarmThreadData*)pData)

  int16_t RMS = 0;
  uint64_t i;
  uint64_t inverseWaitTime;
  for (;;)
  {
    OS_SemaphoreWait(alarmData->voltageCheckSemaphore, 0);
    alarmData->outofRange = true;
    RMS = alarmData->RMS;
    uint16_t VoltageDeviate;

    switch(Mode)
    {
      case DEFINITE:
        for (i =0; i < 5e-9; i+= 1250000)
        {
          if (alarmData->RMS <= VOLT(3) && alarmData->RMS >= VOLT(2))
          {
            alarmData->lowerRequest = false;
            alarmData->voltageAlarm = false;
            alarmData->raiseRequest = false;
            break;
          }
          OS_SemaphoreWait(alarmData->timeCountSemaphore, 0);
        }

        if (alarmData->RMS < VOLT(2))
        {
          alarmData->raiseRequest = true;
          Analog_Put(0, VOLT(5));
          // signal Functionality not working so added analog_put to show logic
          OS_SemaphoreSignal(SignalOutputSemaphore);
        }

        if (alarmData->RMS > VOLT(3))
        {
          alarmData->lowerRequest = true;
          Analog_Put(1, VOLT(5));
          // signal Functionality not working so added analog_put to show logic
          OS_SemaphoreSignal(SignalOutputSemaphore);
        }

      case INVERSE:

        alarmData->TimerDelay = inverseTimerDelay[alarmData->channelNb];
        OutofRange = true;

        VoltageDeviate = VRR_CalcDeviation(alarmData->RMS);
        inverseWaitTime = (uint64_t)((5E-9*VOLT(0.5))/VoltageDeviate);

        for (int i =0; i< inverseWaitTime ; i+= 1250000)
        {
          if (alarmData->RMS <= VOLT(3) && alarmData->RMS >= VOLT(2))
          {
            alarmData->voltageAlarm = false;
            alarmData->lowerRequest = false;
            alarmData->raiseRequest = false;
            break;
          }

          alarmData->RMS = RMS_Calculate(alarmData->sampleArray, MAX_SAMPLE_SIZE);
          VoltageDeviate = VRR_CalcDeviation(alarmData->RMS);
          inverseWaitTime = (uint64_t)((5E-9*VOLT(0.5))/VoltageDeviate);

          OS_SemaphoreWait(PITCountSemaphore, 0);

        }

        if (alarmData->RMS < VOLT(2))
            {
              alarmData->raiseRequest = true;
              Analog_Put(0, VOLT(5));
              // signal Functionality not working so added analog_put to show logic
              OS_SemaphoreSignal(SignalOutputSemaphore);
            }

            if (alarmData->RMS > VOLT(3))
            {
              alarmData->lowerRequest = true;
              Analog_Put(1, VOLT(5));
              // signal Functionality not working so added analog_put to show logic
              OS_SemaphoreSignal(SignalOutputSemaphore);
            }
      }
    }

}




void SignalOutput_Thread(void* pData)
{
  for (;;)
  {
    LEDs_Off(LED_BLUE);
    LEDs_Off(LED_GREEN);
    LEDs_Off(LED_ORANGE);
    LEDs_Off(LED_YELLOW);
    OS_SemaphoreWait(SignalOutputSemaphore,0);

    for (int channelNb = 0; channelNb <NB_ANALOG_CHANNELS ; channelNb++)
    {

      if (AlarmThreadData[channelNb].voltageAlarm)
      {
        //TODO: 0 to Channel 1
        Analog_Put(2, VOLT(5));
//        LEDs_On(LED_GREEN);
      }
      else
      {
        Analog_Put(2, VOLT(0));
      }

      if (AlarmThreadData[channelNb].raiseRequest)
      {
        Analog_Put(1, VOLT(5));
        Flash_AllocateVar(&NbRaises,1);
        NbRaisesCount ++;
        Flash_Write16(NbRaises, NbRaisesCount);
      }
      else
      {
        Analog_Put(1, VOLT(0));
      }

      if (AlarmThreadData[channelNb].lowerRequest)
      {
        Analog_Put(0, VOLT(5));
        Flash_AllocateVar(&NbLowers,1);
        NbLowersCount ++;
        Flash_Write16(NbLowers, NbLowers);
      }
      else
      {
        Analog_Put(0, VOLT(0));
      }

    }


  }

}



/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  OS_ERROR error;

  // Initialise low-level clocks etc using Processor Expert code
   PE_low_level_init();

  // Initialize the RTOS
  OS_Init(CPU_CORE_CLK_HZ, true);

  // Create module initialisation thread
  error = OS_ThreadCreate(InitModulesThread,
                          NULL,
                          &InitModulesThreadStack[THREAD_STACK_SIZE - 1],
                          INIT_MODULES_THREAD_PRIORITY); // Highest priority

  error = OS_ThreadCreate(UART_ReceiveThread,
                          NULL,
                          &UART_RecieveStack[THREAD_STACK_SIZE-1],
                          UART_RECEIVE_THREAD_PRIORITY);

  error = OS_ThreadCreate(UART_TransmitThread,
                          NULL,
                          &UART_TransmitStack[THREAD_STACK_SIZE-1],
                          UART_TRANSMIT_THREAD_PRIORITY);

  error = OS_ThreadCreate(Handle_PacketThread,
                          NULL,
                          &HandlePacketStack[THREAD_STACK_SIZE-1],
                          HANDLE_PACKET_THREAD_PRIORITY);

  error = OS_ThreadCreate(Sample_Thread,
                          NULL,
                          &Sample_Stack[THREAD_STACK_SIZE-1],
                          SAMPLE_THREAD_PRIORITY );

  error = OS_ThreadCreate(SignalOutput_Thread,
                          NULL,
                          &SignalsOutput_Stack[THREAD_STACK_SIZE-1],
                          SIGNALOUT_THREAD_PRIORITY );

  error = OS_ThreadCreate(RMS_CalcThread,
                          NULL,
                          &RMSThreadStack[THREAD_STACK_SIZE - 1],
                          RMS_THREAD_PRIORITY );

// --------------------------------------------------------------------------------------------------------------

  // Create threads for analog loopback channels
  for (uint8_t threadNb = 0; threadNb < NB_ANALOG_CHANNELS; threadNb++)
  {
    error = OS_ThreadCreate(Alarm_Thread,
                            &AlarmThreadData[threadNb],
                            &AlarmThreadStacks[threadNb][THREAD_STACK_SIZE-1],
                            ALARM_THREAD_PRIORITIES[threadNb]);
  }

  // Start multithreading - never returns!
  OS_Start();
}

/*!
 ** @}
 */
