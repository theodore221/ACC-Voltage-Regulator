/*
 * RMS.h
 *
 *  Created on: 22 Jun 2018
 *      Author: 12604120
 */

#ifndef SOURCES_RMS_H_
#define SOURCES_RMS_H_

#include "types.h"

#define MAX_SAMPLE_SIZE 16

int16_t RMS_Calculate (int16_t sampleArray[MAX_SAMPLE_SIZE], int16_t sampleCycleSize);

#endif /* SOURCES_RMS_H_ */
