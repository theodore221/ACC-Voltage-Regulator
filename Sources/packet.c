/*! @file
 *
 *  @brief Routines to implement packet encoding and decoding for the serial port.
 *
 *  contains the functions for implementing the "Tower to PC Protocol" 5-byte packets.
 *
 *  @author 12604120, 12931717
 *  @date LAST EDIT:18/04/2018
 */
/*!
**  @addtogroup Packet_module packet module documentation
**  @{
*/
/* MODULE packet */

#include "types.h"
#include "Cpu.h"

#include "UART.h"
#include "FIFO.h"
#include "packet.h"


#define PACKET_SIZE_BYTES 5

const uint8_t PACKET_ACK_MASK = 0b10000000;

TPacket Packet;

OS_ECB* Packet_ByteReady;

bool Packet_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
  Packet_ByteReady = OS_SemaphoreCreate(0);
  static bool hasInitialized;
  if (hasInitialized)
    return false;

  // Set Packet to initialized if UART initialized and return
  return (hasInitialized = UART_Init(baudRate, moduleClk));
}

void Packet_Get()
{
  static uint8_t packet[5];
  static uint8_t count = 0;
  for (;;)
  {
    UART_InChar(&packet[count]);
    if (count == 4)
    {
      if (packet[4] == packet[0]^packet[1]^packet[2]^packet[3])
      {
	Packet_Command    = packet[0];
	Packet_Parameter1 = packet[1];
	Packet_Parameter2 = packet[2];
	Packet_Parameter3 = packet[3];
	Packet_Checksum   = packet[4];
	count = 0;

	return;
      }
      else
      {
        packet[0] = packet[1];
        packet[1] = packet[2];
	packet[2] = packet[3];
	packet[3] = packet[4];
	count--;
      }
    }
    count++;
  }
}

void Packet_Put(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3)
{
  OS_DisableInterrupts();

  UART_OutChar(command);
  UART_OutChar(parameter1);
  UART_OutChar(parameter2);
  UART_OutChar(parameter3);
  UART_OutChar((command ^ parameter1 ^ parameter2 ^ parameter3));

  OS_EnableInterrupts();
}

/*!
** @}
*/
