/*
 * handle.c
 *
 *  Created on: 30 May 2018
 *      Author: 12604120
 */

#include "Cpu.h"
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"
#include "string.h"

#include "types.h"
#include "packet.h"
#include "LEDs.h"
#include "Flash.h"
#include "PIT.h"
#include "OS.h"
#include "handle.h"

/******************************************************************************\
*                                                                              *
*   Packet Constants                                                           *
*                                                                              *
\******************************************************************************/

// Command Bytes (see Tower Serial Communication Protocol)

static const uint8_t PARAMETER1_TIMING_MODE_GET = 0x00;
static const uint8_t PARAMETER2_TIMING_MODE_GET = 0x00;
static const uint8_t PARAMETER3_TIMING_MODE_GET = 0x00;

static const uint8_t PARAMETER1_TIMING_MODE_SET_DEFINITE = 0x01;
static const uint8_t PARAMETER2_TIMING_MODE_SET_DEFINITE = 0x00;
static const uint8_t PARAMETER3_TIMING_MODE_SET_DEFINITE = 0x00;

static const uint8_t PARAMETER1_TIMING_MODE_SET_INVERSE = 0x02;
static const uint8_t PARAMETER2_TIMING_MODE_SET_INVERSE = 0x00;
static const uint8_t PARAMETER3_TIMING_MODE_SET_INVERSE = 0x00;

static const uint8_t PARAMETER1_NB_RAISES_GET = 0x00;
static const uint8_t PARAMETER2_NB_RAISES_GET = 0x00;
static const uint8_t PARAMETER3_NB_RAISES_GET = 0x00;

static const uint8_t PARAMETER1_NB_RAISES_RESET = 0x01;
static const uint8_t PARAMETER2_NB_RAISES_RESET = 0x00;
static const uint8_t PARAMETER3_NB_RAISES_RESET = 0x00;

static const uint8_t PARAMETER1_NB_LOWERS_GET = 0x00;
static const uint8_t PARAMETER2_NB_LOWERS_GET = 0x00;
static const uint8_t PARAMETER3_NB_LOWERS_GET = 0x00;

static const uint8_t PARAMETER1_NB_LOWERS_RESET = 0x01;
static const uint8_t PARAMETER2_NB_LOWERS_RESET = 0x00;
static const uint8_t PARAMETER3_NB_LOWERS_RESET = 0x00;




extern const uint16_t INIT_SUCCESS;

/******************************************************************************\
*                                                                              *
*   Global Variables                                                           *
*                                                                              *
\******************************************************************************/

extern bool InitSuccess;

uint16union_t * NvTowerNb;
uint16union_t * NvTowerMode;

uint16union_t * RaisesNb;
uint16union_t * LowersNb;

TimerType Mode = DEFINITE;


extern OS_ECB* Packet_ByteReady;

/******************************************************************************\
*                                                                              *
*   Functions                                                                  *
*                                                                              *
\******************************************************************************/

static bool SendTimingModePacket()
{
  Packet_Put
  (
      COMMAND_TIMING_MODE,
      PARAMETER1_TIMING_MODE_GET,
      Mode,
      PARAMETER3_TIMING_MODE_GET
  );

  return true;
}


static bool HandleTimingCommand()
{
  if (Packet_Parameter1 == PARAMETER1_TIMING_MODE_GET && Packet_Parameter2 == PARAMETER2_TIMING_MODE_GET && Packet_Parameter3 == PARAMETER3_TIMING_MODE_GET)
  {
    return SendTimingModePacket();
  }
  else if (Packet_Parameter1 == PARAMETER1_TIMING_MODE_SET_DEFINITE)
  {
    Mode = DEFINITE;
  }
  else if (Packet_Parameter1 == PARAMETER1_TIMING_MODE_SET_INVERSE)
  {
    Mode = INVERSE;
  }
}

static bool SendNbRaisesPacket()
{
  Packet_Put
  (
      COMMAND_NB_RAISES,
      PARAMETER1_NB_RAISES_GET,
      RaisesNb,
      PARAMETER3_NB_RAISES_GET
  );

  return true;
}

static bool HandleNbRaisesCommand()
{
  if (Packet_Parameter1 == PARAMETER1_NB_RAISES_GET && Packet_Parameter2 == PARAMETER2_NB_RAISES_GET && Packet_Parameter3 == PARAMETER3_NB_RAISES_GET)
  {
    return SendNbRaisesPacket();
  }
  else if (Packet_Parameter1 == PARAMETER2_NB_RAISES_RESET)
  {
    return Flash_Write16(RaisesNb, 0);
  }
}

static bool SendNbLowersPacket()
{
  Packet_Put
  (
      COMMAND_NB_LOWERS,
      PARAMETER1_NB_LOWERS_GET,
      LowersNb,
      PARAMETER3_NB_LOWERS_GET
  );

  return true;
}

static bool HandleNbLowersCommand()
{
  if (Packet_Parameter1 == PARAMETER1_NB_RAISES_GET && Packet_Parameter2 == PARAMETER2_NB_RAISES_GET && Packet_Parameter3 == PARAMETER3_NB_RAISES_GET)
  {
    return SendNbLowersPacket();
  }
  else if (Packet_Parameter1 == PARAMETER2_NB_RAISES_RESET)
  {
    return Flash_Write16(LowersNb, 0);
  }
}

static bool HandleFrequencyCommand()
{
  return true;
}

static bool HandleVoltageCommand()
{
  return true;
}

static bool HandleSpectrumCommand()
{
  return true;
}


/*!
 * @brief Attempts to read in Packets and initiate packet commands if packet is valid.
 *
 * @return Void.
 */
static void HandlePacket()
{
  // Status of sent packet.
  bool packetSuccess = false;
  // Was acknowledge requested.
  bool sendAck = false;

  // Does Packet_Command have acknowledge bit set.
  if ((Packet_Command & PACKET_ACK_MASK) != 0)
  {
    sendAck = true;
  }

  // Switch on Packet_Command ignoring acknowledge bit.
  switch (Packet_Command & ~PACKET_ACK_MASK)
  {
    case COMMAND_TIMING_MODE:
    {
      packetSuccess = HandleTimingCommand();
      break;
    }
    case COMMAND_NB_RAISES:
    {
      packetSuccess = HandleNbRaisesCommand();
      break;
    }
    case COMMAND_NB_LOWERS:
    {
      packetSuccess = HandleNbLowersCommand();
      break;
    }
    case COMMAND_FREQUENCY:
    {
      packetSuccess = HandleFrequencyCommand();
      break;
    }
    case COMMAND_VOLTAGE:
    {
      packetSuccess = HandleVoltageCommand();
      break;
    }
    case COMMAND_SPECTRUM:
    {
      packetSuccess = HandleSpectrumCommand();
      break;
    }
    default:
    {
      packetSuccess = false;
      break;
    }
  }

  // Check if FTM0_CH1 was successfully set & valid packet was received

  if (sendAck)
  {
    if (packetSuccess)
    {
      // Add acknowledge bit.
      Packet_Command |= PACKET_ACK_MASK;
    }
    else
    {
      // Remove acknowledge bit.
      Packet_Command &= ~PACKET_ACK_MASK;
    }

    // Send original command back with confirmation of success or failure.
    Packet_Put
    (
      Packet_Command,
      Packet_Parameter1,
      Packet_Parameter2,
      Packet_Parameter3
    );
  }
}

/*!
 * @brief Thread to receive and handle packets.
 *
 * @param pData - void * to satisfy OS_ThreadCreate void (*func)(void *).
 */
void Handle_PacketThread(void * pData)
{
  for (;;)
  {
    OS_SemaphoreWait(Packet_ByteReady, 0);

    if (InitSuccess)
    {
      Packet_Get();
      HandlePacket();
    }
  }
}


