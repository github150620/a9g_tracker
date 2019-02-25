/*
 *
 */
#include "api_network.h"
#include "api_os.h"


HANDLE gsm_lock = NULL;

Network_Location_t gsm_wbuf[10];
uint8_t gsm_wbuf_len = 0;
Network_Location_t gsm_rbuf[10];
uint8_t gsm_rbuf_len = 0;

void GSM_Init() {
    gsm_lock = OS_CreateMutex();
}

void GSM_Update(uint8_t* data, uint8_t number) {
    Network_Location_t* location = (Network_Location_t*)data;
    if (gsm_lock != NULL) {
        OS_LockMutex(gsm_lock);
        gsm_wbuf_len = number;
        memcpy(gsm_wbuf, location, gsm_wbuf_len * sizeof(Network_Location_t));
        OS_UnlockMutex(gsm_lock);
    }
}

void GSM_GetLocation(Network_Location_t** location, uint8_t* len) {
    if (gsm_lock != NULL) {
        OS_LockMutex(gsm_lock);
        gsm_rbuf_len = gsm_wbuf_len;
        memcpy(gsm_rbuf, gsm_wbuf, gsm_rbuf_len * sizeof(Network_Location_t));        
        OS_UnlockMutex(gsm_lock);
    }
    *location = gsm_rbuf;
    *len = gsm_rbuf_len;
}
