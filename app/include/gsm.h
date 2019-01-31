#ifndef __GSM_H__
#define __GSM_H__

#include "api_network.h"

void GSM_Init();
void GSM_Update(Network_Location_t* location, uint8_t number);
void GSM_GetLocation(Network_Location_t** location, uint8_t* len);

#endif
