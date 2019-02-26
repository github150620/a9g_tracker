#ifndef __GPS_FILTER_H__
#define __GPS_FILTER_H__

#include "stdbool.h"

bool GPS_IsInChina(float latitude, float longitude);
bool GPS_IsPossible(int timestamp, float latitude, float longitude);

#endif