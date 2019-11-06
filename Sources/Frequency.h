/*
 * Frequency.h
 *
 *  Created on: 27 Jun 2018
 *      Author: theod
 */

#ifndef SOURCES_FREQUENCY_H_
#define SOURCES_FREQUENCY_H_

#include "types.h"

#define VOLT(x) 3277*(x)

bool Frequency_isZeroCrossing (int16_t sample1, int16_t sample2);

void Frequency_interpolatePoint1 (int16_t sample1, int16_t sample2);



#endif /* SOURCES_FREQUENCY_H_ */
