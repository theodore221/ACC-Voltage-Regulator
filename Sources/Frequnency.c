/*
 * Frequnency.c
 *
 *  Created on: 27 Jun 2018
 *      Author: theod
 */

#include "Frequency.h"



bool Frequency_isZeroCrossing (int16_t sample1, int16_t sample2)
{
  if (((sample1 > VOLT(0)) && (sample2 < VOLT(0))) || ((sample1 < VOLT(0)) && (sample2 > VOLT(0))))
  {
    return true;
  }
  else
  {
    return false;
  }

}

void Frequency_interpolatePoint1 (int16_t sample1, int16_t sample2)
{

}


