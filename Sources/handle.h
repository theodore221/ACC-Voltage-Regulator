/*
 * handle.h
 *
 *  Created on: 30 May 2018
 *      Author: 12604120
 */

#ifndef SOURCES_HANDLE_H_
#define SOURCES_HANDLE_H_

#include "types.h"

typedef enum {
  COMMAND_TIMING_MODE = 0x10,
  COMMAND_NB_RAISES = 0x11,
  COMMAND_NB_LOWERS = 0x12,
  COMMAND_FREQUENCY = 0x17,
  COMMAND_VOLTAGE   = 0x18,
  COMMAND_SPECTRUM  = 0x19
} PacketCommand_t;


typedef enum
{
  DEFINITE,
  INVERSE
} TimerType;



bool Handle_SendStartupPackets(void);

void Handle_PacketThread(void * pd);

#endif /* SOURCES_HANDLE_H_ */
