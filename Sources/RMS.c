/*
 * RMS.c
 *
 *  Created on: 22 Jun 2018
 *      Author: 12604120
 */

#include "RMS.h"
#include <Math.h>
#include <stdbool.h>

int16_t RMS_Calculate (int16_t sampleArray[MAX_SAMPLE_SIZE], int16_t sampleCycleSize)
{
  float rmsSum;
  float rmsAverage;
  float rmsRoot;

  for (int Count =0; Count < sampleCycleSize; Count++)
  {
    rmsSum += (sampleArray[Count]*sampleArray[Count]);
  }

  rmsAverage = rmsSum / sampleCycleSize;
  rmsRoot = sqrt(rmsAverage);

  return (int16_t)rmsRoot;
}

