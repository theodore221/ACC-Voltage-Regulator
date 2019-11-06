/*! @file
 *
 *  @brief I/O routines for UART communications on the TWR-K70F120M.
 *
 *  This contains the functions for operating the UART (serial port).
 *
 *  @author Benjamin Essam
 *  @date 2018-05-30
 */
/*!
**  @addtogroup UART_module UART module documentation
**  @{
*/
/* MODULE UART */

#include "UART.h"
#include "Cpu.h"
#include "OS.h"
#include "FIFO.h"

static FIFO_t RxFIFO;
static FIFO_t TxFIFO;

static OS_ECB* ByteReceived;
static OS_ECB* TransmitReady;
static uint8_t RxByte;

const uint8_t BAUD_MULTIPLIER = 32;

extern OS_ECB* Packet_ByteReady;

bool UART_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
  // init private globals
  ByteReceived = OS_SemaphoreCreate(0);
  TransmitReady = OS_SemaphoreCreate(0);
  FIFO_Init(&RxFIFO);
  FIFO_Init(&TxFIFO);

  // baud rate 0 turns off UART
  if (baudRate != 0)
  {
    uint8_t BRFA;
    uint16union_t SBR;
    uint16_t baudRateCalc;

    // Calculate UART Baud Rate Bits (SBR) and Baud Rate Fine Adjust (BRFA)
    baudRateCalc = 2*(moduleClk/(baudRate));

    SBR.l = baudRateCalc / BAUD_MULTIPLIER;
    BRFA  = baudRateCalc % BAUD_MULTIPLIER;

    // Enable UART2
    SIM_SCGC4 |= SIM_SCGC4_UART2_MASK;
    // Enable Port E
    SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK;

    // Set PTE16 to UART2_Tx (see pin multiplexing table)
    PORTE_PCR16 = PORT_PCR_MUX(3);
    // Set PTE17 to UART2_Rx (see pin multiplexing table)
    PORTE_PCR17 = PORT_PCR_MUX(3);

    // Initialize baud rate
    UART2_BDH = UART_BDH_SBR(SBR.s.Hi);
    UART2_BDL = UART_BDL_SBR(SBR.s.Lo);

    UART2_C4 |= UART_C4_BRFA_MASK;
    UART2_C4 &= (BRFA |= ~(UART_C4_BRFA_MASK));

    // Set receiver interrupt flag
    UART2_C2 |= UART_C2_RIE_MASK;
    UART2_C2 &= ~UART_C2_TIE_MASK;

    // Initialize transmitter and receiver
    UART2_C2 |= UART_C2_RE_MASK;
    UART2_C2 |= UART_C2_TE_MASK;

    OS_DisableInterrupts();
    // Enable UART NVIC
    // 49 mod 32 = 17;
    NVICICPR1 = (1 << 17);
    NVICISER1 = (1 << 17);
    OS_EnableInterrupts();

    return true;
  }

  return false;
}

void UART_InChar(uint8_t * const dataPtr)
{
//  UART2_C2 |= UART_C2_RIE_MASK;
  FIFO_Get(&RxFIFO, dataPtr);
//  //Use RxFIFO to get the incoming Bytes from the PC
//if (FIFO_Get(&RxFIFO, dataPtr))
//{
//  // Set RIE Flag
//  UART2_C2 |= UART_C2_RIE_MASK; // never turn off
//  return true;
//}
//else
//  return false;
}

void UART_OutChar(const uint8_t data)
{
  FIFO_Put(&TxFIFO, data);
  UART2_C2 |= UART_C2_TIE_MASK;
}


void UART_ReceiveThread(void* pData)
{
  for (;;)
  {
    OS_ERROR error = OS_SemaphoreWait(ByteReceived, 0);

    if (error == OS_NO_ERROR)
    {
      FIFO_Put(&RxFIFO,RxByte);
      OS_SemaphoreSignal(Packet_ByteReady);
    }
  }
}

void UART_TransmitThread(void* pData)
{
  for (;;)
  {
    OS_ERROR error;
    error = OS_SemaphoreWait(TransmitReady, 0);
    if (error == OS_NO_ERROR)
    {
      if (UART2_S1 & UART_S1_TDRE_MASK)
      {
	FIFO_Get(&TxFIFO,(uint8_t*)&UART2_D);
	UART2_C2 |= UART_C2_TIE_MASK;
      }
    }
  }
}


/*! @brief Interrupt service routine for the UART.
 *  @return Void.
 *  @note Assumes the UART has been initialised.
 */
void __attribute__ ((interrupt)) UART_ISR(void)
{
  OS_ISREnter();

  // To replace the Poll
  // Should do the same thing, except with interrupt flags
  uint8_t statusReg = UART2_S1;
  uint8_t controlReg = UART2_C2;

  // Receive thread with semaphores
  // Check RIE Flag
  if (controlReg & UART_C2_RIE_MASK)
  {
    if (statusReg & UART_S1_RDRF_MASK)
    {
    //Check if there is a Byte coming in, then put that info
      RxByte = UART2_D;
      OS_SemaphoreSignal(ByteReceived);
    }
  }

  // Transmit thread with semaphores
  if (controlReg & UART_C2_TIE_MASK)
  {
    //Check if there is a Byte going out, then get that info
    if (statusReg & UART_S1_TDRE_MASK)
    {
      UART2_C2 &= ~UART_C2_TIE_MASK;
      OS_SemaphoreSignal(TransmitReady);
    }
  }

  OS_ISRExit();
}

void UART_Poll(void)
{
  uint8_t statusReg = UART2_S1;
  //Check if there is a Byte coming in, then put that info
  if (statusReg & UART_S1_RDRF_MASK)
  {
    FIFO_Put(&RxFIFO,UART2_D);
  }

  //Check if there is a Byte going out, then get that info
  if (statusReg & UART_S1_TDRE_MASK)
  {
    FIFO_Get(&TxFIFO,(uint8_t*)&UART2_D);
  }
}

/* END UART */
/*!
** @}
*/
