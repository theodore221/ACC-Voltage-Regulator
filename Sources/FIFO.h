/*! @file
 *
 *  @brief Routines to implement a FIFO buffer.
 *
 *  This contains the structure and "methods" for accessing a byte-wide FIFO.
 *
 *  @author PMcL
 *  @date 2015-07-23
 */

#ifndef FIFO_H
#define FIFO_H

#include "types.h"
#include "OS.h"

// Size of the FIFO's internal Buffer
#define FIFO_SIZE 256

/*!
 * @struct FIFO_t
 */
typedef struct
{
  uint16_t Start;		/*!< The index of the position of the oldest data in the FIFO */
  uint16_t End; 		/*!< The index of the next available empty position in the FIFO */
  OS_ECB* FreeSpace;            /*!< Semaphore indicating the amount of free space in the FIFO */
  OS_ECB* ItemsAvailable;       /*!< Semaphore indicating the amount of space used in the FIFO */
  OS_ECB* IsFree;               /*!< Semaphore indicating the FIFO is not in use */
  uint8_t Buffer[FIFO_SIZE];	/*!< The actual array of bytes to store the data */
} FIFO_t;

/*! @brief Initialize the FIFO before first use.
 *
 *  @param FIFO A pointer to the FIFO that needs initializing.
 *  @return void
 */
void FIFO_Init(FIFO_t * const FIFO);

/*! @brief Put one character into the FIFO.
 *
 *  @param FIFO A pointer to a FIFO struct where data is to be stored.
 *  @param data A byte of data to store in the FIFO buffer.
 *  @return bool - TRUE if data is successfully stored in the FIFO.
 *  @note Assumes that FIFO_Init has been called.
 */
void FIFO_Put(FIFO_t * const FIFO, const uint8_t data);

/*! @brief Get one character from the FIFO.
 *
 *  @param FIFO A pointer to a FIFO struct with data to be retrieved.
 *  @param dataPtr A pointer to a memory location to place the retrieved byte.
 *  @return bool - TRUE if data is successfully retrieved from the FIFO.
 *  @note Assumes that FIFO_Init has been called.
 */
void FIFO_Get(FIFO_t * const FIFO, uint8_t * const dataPtr);

#endif
