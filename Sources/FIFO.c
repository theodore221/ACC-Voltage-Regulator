/*! @file
 *
 *  @brief Routines to implement a FIFO buffer.
 *
 *  This contains the structure and "methods" for accessing a byte-wide FIFO.
 *
 *  @author Ben Essam, Theodore Xavier
 *  @date 2018-05-01
 */
/*!
**  @addtogroup FIFO_module FIFO module documentation
**  @{
*/
/* MODULE FIFO */

#include "Cpu.h"
#include "FIFO.h"
#include "OS.h"

void FIFO_Init(FIFO_t * const FIFO)
{
  FIFO->Start = 0;
  FIFO->End = 0;
  FIFO->FreeSpace = OS_SemaphoreCreate(FIFO_SIZE);
  FIFO->ItemsAvailable = OS_SemaphoreCreate(0);

}

void FIFO_Put(FIFO_t * const FIFO, const uint8_t data)
{
  OS_SemaphoreWait(FIFO->FreeSpace, 0);

  FIFO->Buffer[FIFO->End] = data;
  FIFO->End++;
  if (FIFO->End >= FIFO_SIZE - 1)
    FIFO->End = 0;

  OS_SemaphoreSignal(FIFO->ItemsAvailable);
}

void FIFO_Get(FIFO_t * const FIFO, uint8_t * const dataPtr)
{
  OS_SemaphoreWait(FIFO->ItemsAvailable, 0);

  *dataPtr = FIFO->Buffer[FIFO->Start];

  FIFO->Start++;
  if (FIFO->Start == FIFO_SIZE - 1)
    FIFO->Start = 0;

  OS_SemaphoreSignal(FIFO->FreeSpace);
}

/*!
** @}
*/
