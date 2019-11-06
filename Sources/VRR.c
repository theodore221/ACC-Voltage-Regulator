/*
 * VRR.c
 *
 *  Created on: 24 Jun 2018
 *      Author: theod
 */

#include "VRR.h"

#define VOLT(x) 3276.7*(x)

bool VRR_CheckLimits(int16_t value)
{

  return (value > VOLT(3) || value < VOLT(2));

}

int16_t VRR_CalcDeviation(int16_t value)
{
  if (value > VOLT(2.5))
  {
    return value - VOLT(2.5);
  }
  if (value < VOLT(2.5))
  {
    return VOLT(2.5) - value;
  }
}



