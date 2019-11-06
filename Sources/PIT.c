/*! @file
 *
 *  @brief Routines for controlling Periodic Interrupt Timer (PIT) on the TWR-K70F120M.
 *
 *  This contains the functions for operating the periodic interrupt timer (PIT).
 *
 *  @author Jibin Mathew
 *  @date 2018-04-24
 */
/*!
 * @addtogroup PIT_module PIT module documentation
 * @{
 */
/* MODULE PIT */

#include <stdio.h>

#include "PIT.h"
#include "MK70F12.h"
#include "OS.h"
#include "types.h"
#include "Cpu.h"


static uint32_t ModuleClk;
static void (*CallbackFuncPtr)(void *);
static void * CallbackArgs;



OS_ECB* PITSemaphore;


/*! @brief PIT initialising function
 *
 *  Creates first new timed interrupt
 *  Turns on Green LED
 *  @return true if it works
 */
bool PIT_Init(const uint32_t moduleClk, void (*userFunction)(void*), void* userArguments)
{
  ModuleClk = moduleClk;
  CallbackFuncPtr = userFunction;
  CallbackArgs = userArguments;
  PITSemaphore = OS_SemaphoreCreate(0);

  //Initializing system clock gate for PIT
  SIM_SCGC6 |= SIM_SCGC6_PIT_MASK;
  //Clears the MDIS bit to enable clock for PIT Timer
  PIT_MCR &= ~PIT_MCR_MDIS_MASK;
  //Freezes the timer when debugging
  PIT_MCR = PIT_MCR_FRZ_MASK;

  //Clearing the PIT interrupt flag to stop interrupts when initializing
  PIT_TFLG0 |= PIT_TFLG_TIF_MASK;

  //Enables PIT Timer
  PIT_Enable(true);

  //Enables Timer Interrupt
  PIT_TCTRL0 |= PIT_TCTRL_TIE_MASK;

  //Initialize NVIC; Vector = 84; IRQ = 68 non-IPR = 2; IRQ mod 32
  EnterCritical();
  //Clear pending interrupts
  NVICICPR2 = (1 << 4);
  //Enable Interrupts for PIT module
  NVICISER2 = (1 << 4);
  ExitCritical();

  return true;

}

/*! @brief PIT Set new timed interrupt
 *
 *  Creates new timed interrupt
 *  Turns on Green LED
 *  @return true if it works
 */
void PIT_Set(const uint32_t period, const bool restart)
{
  uint32_t denom = 1e9/ModuleClk;
  uint32_t LDVAL = period/denom  - 1;
  PIT_LDVAL0 = LDVAL;

  if (restart)
  {
    PIT_Enable(false);
    PIT_LDVAL0 = LDVAL;
    PIT_Enable(true);
  }
}

void PIT_Enable(const bool enable)
{
  if (enable)
  {
    PIT_TCTRL0 |= PIT_TCTRL_TEN_MASK;
  }
  else
  {
    PIT_TCTRL0 &= ~PIT_TCTRL_TEN_MASK;
  }
}


void __attribute__ ((interrupt)) PIT_ISR(void)
{
  OS_ISREnter();

  PIT_TFLG0 |= PIT_TFLG_TIF_MASK;

  OS_SemaphoreSignal(PITSemaphore);

//  if (UserFunctionPtr)
//    (*UserFunctionPtr)(UserArgumentsPtr);


  OS_ISRExit();
}

/*!
 ** @}
 */

