/*
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include "api_debug.h"
#include "api_event.h"
#include "api_hal_gpio.h"
#include "api_hal_pm.h"
#include "api_info.h"
#include "api_key.h"
#include "api_network.h"
#include "api_os.h"
#include "api_sim.h"
#include "api_socket.h"
#include "minmea.h"

#include "gps.h"
#include "gps_parse.h"

#include "gsm.h"
#include "log.h"
#include "led.h"
#include "sock.h"

#define VERSION "0.3.24"

#define SERVER_HOST "tracker.fish2bird.com"
#define SERVER_PORT 19999

#define RECEIVE_BUFFER_MAX_LENGTH 256
#define SEND_BUFFER_MAX_LENGTH    256

#define AppMain_TASK_STACK_SIZE    (2048 * 2)
#define AppMain_TASK_PRIORITY      2

#define POWER_TASK_STACK_SIZE      (1024 * 1)
#define POWER_TASK_PRIORITY        4

#define Network_TASK_STACK_SIZE    (1024 * 1)
#define Network_TASK_PRIORITY      6

#define Loop_TASK_STACK_SIZE       (2048 * 2)
#define Loop_TASK_PRIORITY         8

#define Display_TASK_STACK_SIZE   (1024 * 1)
#define Display_TASK_PRIORITY      10

#define LOG_FILE_PATH "/t/1.log"
#define GPS_NMEA_LOG_FILE_PATH "/t/gps_nmea.log"

char imei[16];
char iccid[21];
char imsi[16];

HANDLE mainTaskHandle  = NULL;
HANDLE otherTaskHandle = NULL;
HANDLE networkTaskHandle = NULL;
HANDLE blinkTaskHandle = NULL;

HANDLE networkCellInfoEventHandle = NULL;

bool isNetworkRegistered = false;
bool isNetworkRegisterDenied = false;
bool isNetworkAttached = false;
bool isNetworkActivated = false;
bool isSocketConnected = false;

int startAttachCount = 0;
int startActiveCount = 0;

uint8_t rbuf[RECEIVE_BUFFER_MAX_LENGTH];
uint8_t wbuf[SEND_BUFFER_MAX_LENGTH];

GPS_Info_t gpsInfoBuf;



void GPS_Filter(struct minmea_sentence_rmc *rmc) {
    static int latestTime = 0;
    static float latestLat = 0.0;
    static float latestLng = 0.0;

    const float pi = 3.141592653;
    const float r = 6371; // 6371km
    const float speed = 240; // 240km/h
    const float gpsInterval = 5; // 5s
    float delta = (speed*(gpsInterval/3600))/(2*pi*r)*360;
    int t = time(NULL);
    float lat = minmea_tocoord(&rmc->latitude);
    float lng = minmea_tocoord(&rmc->longitude);
    if (lat-latestLat<delta && lat-latestLat>-1.0*delta && lng-latestLng<delta && lng-latestLng>-1.0*delta) {
        memcpy(&gpsInfoBuf.rmc, rmc, sizeof(gpsInfoBuf.rmc));
    }
    latestTime = t;
    latestLat = lat;
    latestLng = lng;
}

void SendSIM() {
    memset(rbuf, 0, sizeof(rbuf));
    memset(wbuf, 0, sizeof(wbuf));
    snprintf(wbuf, sizeof(wbuf), "$SIM:%s,%s,%s,%s\n", imei, iccid, imsi,VERSION);
    log_print(wbuf);
    LED_TurnOn(LED_LED1);
    sock_request(wbuf, strlen(wbuf), rbuf, RECEIVE_BUFFER_MAX_LENGTH);
    LED_TurnOff(LED_LED1);
    log_print(rbuf);
}

void SendPM(time_t t, uint8_t percent, uint16_t voltage) {
    memset(rbuf, 0, sizeof(rbuf));
    memset(wbuf, 0, sizeof(wbuf));
    snprintf(wbuf, sizeof(wbuf), "$PM:%d,%d,%d\n", t, voltage, percent);
    log_print(wbuf);
    LED_TurnOn(LED_LED1);    
    sock_request(wbuf, strlen(wbuf), rbuf, RECEIVE_BUFFER_MAX_LENGTH);
    LED_TurnOff(LED_LED1);
    log_print(rbuf);
}

void SendGSM(time_t t, Network_Location_t *nl) {
    memset(rbuf, 0, sizeof(rbuf));
    memset(wbuf, 0, sizeof(wbuf));
    snprintf(wbuf, sizeof(wbuf), "$GSM:%d,%d%d%d,%d,%d,%d,%d,%d,%d,%d\n",
                                    t,
                                    nl->sMcc[0],
                                    nl->sMcc[1],
                                    nl->sMcc[2],
                                    nl->sMnc[0]*100 + nl->sMnc[1]*10 + nl->sMnc[2],
                                    nl->sLac,
                                    nl->sCellID,
                                    nl->iBsic,
                                    nl->iRxLev,
                                    nl->iRxLevSub,
                                    nl->nArfcn);
    log_print(wbuf);
    LED_TurnOn(LED_LED1); 
    sock_request(wbuf, strlen(wbuf), rbuf, RECEIVE_BUFFER_MAX_LENGTH);
    LED_TurnOff(LED_LED1);
    log_print(rbuf);
}

void SendGPS(time_t t, struct minmea_sentence_rmc *rmc) {
    memset(rbuf, 0, sizeof(rbuf));
    memset(wbuf, 0, sizeof(wbuf));
    float latitude = minmea_tocoord(&rmc->latitude);
    float longitude = minmea_tocoord(&rmc->longitude);
    float speed = minmea_tofloat(&rmc->speed);
    float course = minmea_tofloat(&rmc->course);    
    snprintf(wbuf, sizeof(wbuf), "$GPS:%d,%02d%02d%02d,%.7f,%.7f,%.1f,%.1f,%d\n",
                                    t,
                                    rmc->time.hours,
                                    rmc->time.minutes,
                                    rmc->time.seconds,
                                    latitude,
                                    longitude,
                                    speed,
                                    course,
                                    rmc->valid);
    log_print(wbuf);
    LED_TurnOn(LED_LED1);    
    sock_request(wbuf, strlen(wbuf), rbuf, RECEIVE_BUFFER_MAX_LENGTH);
    LED_TurnOff(LED_LED1);    
    log_print(rbuf);
}

void NetworkCallback(Network_Status_t status) {
    char buf[32];
    sprintf(buf, "NetworkCallback(): %d", status);
    log_print(buf);
}

void DisplayTask(VOID *pData) {
    while (1) {
        if (isSocketConnected) {
            LED_SetBlink(LED_LED1, LED_BLINK_FREQ_1HZ, LED_BLINK_DUTY_EMPTY);
        } else if (isNetworkActivated) {
            LED_SetBlink(LED_LED1, LED_BLINK_FREQ_1HZ, LED_BLINK_DUTY_HALF);
        } else if (isNetworkAttached) {
            LED_SetBlink(LED_LED1, LED_BLINK_FREQ_2HZ, LED_BLINK_DUTY_HALF);
        } else if (isNetworkRegistered) {
            LED_SetBlink(LED_LED1, LED_BLINK_FREQ_4HZ, LED_BLINK_DUTY_HALF);
        } else {
            LED_SetBlink(LED_LED1, LED_BLINK_FREQ_8HZ, LED_BLINK_DUTY_HALF);
        }
        OS_Sleep(1000);
    }
}


HANDLE gps_lock = NULL;

void LoopTask(VOID *pData) {
    //PM_SetSysMinFreq(PM_SYS_FREQ_13M);
    //PM_SetSysMinFreq(PM_SYS_FREQ_78M);
    


    log_init(LOG_FILE_PATH);
    GSM_Init();
    sock_init(SERVER_HOST, SERVER_PORT);

    while (1) {
        uint8_t status = 0;
        if (isNetworkRegistered && Network_GetActiveStatus(&status) && status==1) {
            break;
        }
        OS_Sleep(1000);
    }

    GPS_Init();
    GPS_SaveLog(true, GPS_NMEA_LOG_FILE_PATH);
    GPS_Open(NULL);
    GPS_SetOutputInterval(5000); // It doesn't work one time. 
    GPS_SetOutputInterval(5000);
    GPS_SetOutputInterval(5000);
    LED_SetBlink(LED_LED2, LED_BLINK_FREQ_0, LED_BLINK_DUTY_HALF);
    //if(!GPS_AGPS(29.50191, 106.56246, 0, true)) {
    //    Trace(1,"agps fail");
    //}    

    memset(imei,0,sizeof(imei));
    INFO_GetIMEI(imei);
    Trace(1,"imei:%s",imei);

    memset(iccid,0,sizeof(iccid));
    SIM_GetICCID(iccid);
    Trace(1,"ICCID: %s", iccid);

    memset(imsi,0,sizeof(imsi));
    SIM_GetIMSI(imsi);
    Trace(1, "IMSI: %s", imsi);

    networkCellInfoEventHandle = OS_CreateSemaphore(0);

    int t;
    int pmLatest = 0;
    int gsmLatest = 0;
    int gpsLatest = 0;
    int fullGsmLastest = 0;

    while(1)
    {
        t = time(NULL) + 3600*8;
        if (t-pmLatest >= 60 ) {
            log_print("send PM data");
            Trace(1, "send PM data");
            t = time(NULL) + 3600*8;
            uint8_t p;
            uint16_t v = PM_Voltage(&p);
            if (sock_status() == 1) {
                SendPM(t, p, v);
                pmLatest = t;
            } else {
                if (sock_connect()) {
                    isSocketConnected = true;
                    SendSIM();
                    SendPM(t, p, v);
                    pmLatest = t;
                } else {
                    isSocketConnected = false;
                }
            }
        }

        t = time(NULL) + 3600*8;
        if (t-gpsLatest >= 30) {
            log_print("send gps data");
            if (sock_status() == 1) {
                SendGPS(t, &gpsInfoBuf.rmc);
                gpsLatest = t;
            } else {
                if (sock_connect()) {
                    isSocketConnected = true;
                    SendSIM();
                    SendGPS(t, &gpsInfoBuf.rmc);
                    gpsLatest = t;
                } else {
                    isSocketConnected = false;
                }
            }
        }

        t = time(NULL) + 3600*8;
        if (t-gsmLatest >= 60) {
            log_print("send GSM data");
            Trace(1, "send GSM data");
            if (Network_GetCellInfoRequst()) {
                if (OS_WaitForSemaphore(networkCellInfoEventHandle, 5000)) {
                    t = time(NULL) + 3600*8;
                    Network_Location_t* p;
                    uint8_t l;
                    GSM_GetLocation(&p, &l);
                    if (sock_status() == 1) {
                        if (t-fullGsmLastest>=600) {
                            for (int i=0;i<l;i++) {
                                SendGSM(t, &p[i]);
                            }
                            gsmLatest = t;
                            fullGsmLastest = t;
                        } else {
                            for (int i=0;i<l;i++) {
                                SendGSM(t, &p[i]);
                            }
                            gsmLatest = t;
                        }
                    } else {
                        if (sock_connect()) {
                            isSocketConnected = true;
                            SendSIM();
                            if (t-fullGsmLastest>=600) {
                                for (int i=0;i<l;i++) {
                                    SendGSM(t, &p[i]);
                                }
                                gsmLatest = t;
                                fullGsmLastest = t;
                            } else {
                                for (int i=0;i<l;i++) {
                                    SendGSM(t, &p[i]);
                                    break;
                                }
                                gsmLatest = t;
                            }
                        } else {
                            isSocketConnected = false;
                        }
                    }
                } else {
                    log_print("OS_WaitForSemaphore()...timeout");
                }
            } else {
                log_print("Network_GetCellInfoRequst()...failed");
            }
        }

        //PM_SleepMode(true);
        PM_SetSysMinFreq(PM_SYS_FREQ_13M);
        OS_Sleep(30000);
        PM_SetSysMinFreq(PM_SYS_FREQ_312M);
        //PM_SleepMode(false);
    }
}

void NetworkManageTask(VOID *pData) {
    uint8_t attachStatus = 0;
    uint8_t activateStatus = 0;

    TIME_SetIsAutoUpdateRtcTime(true);
    Network_SetStatusChangedCallback(NetworkCallback);

    while(1) {
        if (isNetworkRegisterDenied) {
            OS_Sleep(10000);
            continue;
        }

        if (!isNetworkRegistered) {
            OS_Sleep(3000);
            continue;
        }

        if (!Network_GetAttachStatus(&attachStatus)) {
            log_print("Network_GetAttachStatus()...false");
            OS_Sleep(3000);
            continue;
        }

        if (attachStatus==0) {
            isNetworkAttached = false;
            if (startAttachCount > 10) {
                log_print("startAttachCount>10, PM_Restart()");
                PM_Restart();
            }
            Network_StartAttach();
            startAttachCount++;
            log_print("Network_StartAttach()");
            OS_Sleep(5000);
            continue;
        } else if (attachStatus==1) {
            log_print("attachStatus==1");
            isNetworkAttached = true;
            startAttachCount = 0;
        } else {
            log_print("unkown attach status");
            OS_Sleep(3000);
            continue;
        }

        if (!Network_GetActiveStatus(&activateStatus)) {
            log_print("Network_GetActiveStatus()...false");
            OS_Sleep(3000);
            continue;
        }

        if (activateStatus==0) {
            isNetworkActivated = false;
            if ( startActiveCount > 10) {
                log_print("startActiveCount>10, PM_Restart()");
                PM_Restart();
            }
            Network_PDP_Context_t context = {
                .apn        ="cmiot",
                .userName   = ""    ,
                .userPasswd = ""
            };
            Network_StartActive(context);
            startActiveCount++;
            log_print("Network_StartActive()");
            OS_Sleep(10000);
            continue;
        } else if (activateStatus==1) {
            log_print("activateStatus==1");
            isNetworkActivated = false;
            startActiveCount = 0;
            OS_Sleep(30000);
        } else {
            log_print("unkown active status");
            OS_Sleep(3000);
            continue;            
        }
    }
}

void PowerManageTask(VOID *pData) {
    uint8_t p;
    uint16_t v;
    while (1) {
        v = PM_Voltage(&p);
        if ( v < 3600 ) {
            PM_ShutDown();
        }
        OS_Sleep(60000);
    }
}

void EventDispatch(API_Event_t* pEvent)
{
    int keyDownAt = 0;
    char buf[64];

    switch(pEvent->id)
    {
        case API_EVENT_ID_SYSTEM_READY:
            Trace(1, "API_EVENT_ID_SYSTEM_READY");
            log_print("API_EVENT_ID_SYSTEM_READY");
            break;
        case API_EVENT_ID_KEY_DOWN:
            Trace(1,"key down, key:0x%02x",pEvent->param1);
            if(pEvent->param1 == KEY_POWER) {
                keyDownAt = time(NULL);
            }
            break;
        case API_EVENT_ID_KEY_UP:
            Trace(1,"key release, key:0x%02x",pEvent->param1);
            if(pEvent->param1 == KEY_POWER) {
                if ( time(NULL) - keyDownAt > 5) {
                    LED_SetBlink(LED_LED1, LED_BLINK_FREQ_1HZ, LED_BLINK_DUTY_FULL);
                    LED_SetBlink(LED_LED2, LED_BLINK_FREQ_1HZ, LED_BLINK_DUTY_FULL);
                    OS_Sleep(3000);
                    PM_ShutDown();
                }
            }
            break;
        case API_EVENT_ID_NO_SIMCARD:
            Trace(1, "API_EVENT_ID_NO_SIMCARD");
            break;
        case API_EVENT_ID_NETWORK_REGISTERED_HOME:
        case API_EVENT_ID_NETWORK_REGISTERED_ROAMING:
            Trace(1, "API_EVENT_ID_NETWORK_REGISTERED_xxxx %d", pEvent->id);
            log_print("API_EVENT_ID_NETWORK_REGISTERED_xxxx");
            isNetworkRegistered = true;
            isNetworkRegisterDenied = false;
            break;
        case API_EVENT_ID_NETWORK_REGISTER_SEARCHING:
            Trace(1, "API_EVENT_ID_NETWORK_REGISTER_SEARCHING");
            log_print("API_EVENT_ID_NETWORK_REGISTER_SEARCHING");
            isNetworkRegistered = false;
            break;
        case API_EVENT_ID_NETWORK_REGISTER_DENIED:
            Trace(1, "API_EVENT_ID_NETWORK_REGISTER_DENIED");
            log_print("API_EVENT_ID_NETWORK_REGISTER_DENIED");
            isNetworkRegistered = false;
            isNetworkRegisterDenied = true;
            break;
        case API_EVENT_ID_NETWORK_DETACHED:
            Trace(1, "API_EVENT_ID_NETWORK_DETACHED");
            log_print("API_EVENT_ID_NETWORK_DETACHED");
            break;
        case API_EVENT_ID_NETWORK_ATTACH_FAILED:
            Trace(1, "API_EVENT_ID_NETWORK_ATTACH_FAILED");
            log_print("API_EVENT_ID_NETWORK_ATTACH_FAILED");
            break;            
        case API_EVENT_ID_NETWORK_ATTACHED:
            Trace(1, "API_EVENT_ID_NETWORK_ATTACHED");
            log_print("API_EVENT_ID_NETWORK_ATTACHED");
            break;
        case API_EVENT_ID_NETWORK_DEACTIVED:
            Trace(1, "API_EVENT_ID_NETWORK_DEACTIVED");
            log_print("API_EVENT_ID_NETWORK_DEACTIVED");
            break;            
        case API_EVENT_ID_NETWORK_ACTIVATE_FAILED:
            Trace(1, "API_EVENT_ID_NETWORK_ACTIVATE_FAILED");
            log_print("API_EVENT_ID_NETWORK_ACTIVATE_FAILED");
            break;            
        case API_EVENT_ID_NETWORK_ACTIVATED:
            Trace(1, "API_EVENT_ID_NETWORK_ACTIVATED");
            log_print("API_EVENT_ID_NETWORK_ACTIVATED");
            break;
        case API_EVENT_ID_NETWORK_GOT_TIME: {
            RTC_Time_t* t = (RTC_Time_t*)pEvent->pParam1;
            sprintf(buf, "NETWORK_GOT_TIME %04d-%02d-%02d %02d:%02d:%02d+%d,%d", t->year, t->month, t->day, t->hour, t->minute, t->second, t->timeZone, t->timeZoneMinutes);
            log_print(buf);
            break;
        }
        case API_EVENT_ID_NETWORK_CELL_INFO:
            Trace(1, "API_EVENT_ID_NETWORK_CELL_INFO");
            log_print("API_EVENT_ID_NETWORK_CELL_INFO");
            GSM_Update(pEvent->pParam1, pEvent->param1);
            if (networkCellInfoEventHandle != NULL) {
                OS_ReleaseSemaphore(networkCellInfoEventHandle);
            }
            break;
        case API_EVENT_ID_GPS_UART_RECEIVED:
            // Trace(1,"received GPS data,length:%d, data:%s,flag:%d",pEvent->param1,pEvent->pParam1,flag);
            LED_TurnOn(LED_LED2);
            GPS_Update(pEvent->pParam1,pEvent->param1);
            GPS_Filter(&Gps_GetInfo()->rmc);
            LED_TurnOff(LED_LED2);
            break; 
        default:
            break;
    }
}

void AppMainTask(VOID *pData)
{
    LED_Init();
    LED_SetBlink(LED_LED1, LED_BLINK_FREQ_1HZ, LED_BLINK_DUTY_FULL);
    LED_SetBlink(LED_LED2, LED_BLINK_FREQ_1HZ, LED_BLINK_DUTY_FULL);

    API_Event_t* event=NULL;
    OS_CreateTask(PowerManageTask, NULL, NULL, POWER_TASK_STACK_SIZE, POWER_TASK_PRIORITY, 0, 0, "power task");
    OS_CreateTask(NetworkManageTask, NULL, NULL, Network_TASK_STACK_SIZE, Network_TASK_PRIORITY, 0, 0, "network Task");
    OS_CreateTask(LoopTask, NULL, NULL, Loop_TASK_STACK_SIZE, Loop_TASK_PRIORITY, 0, 0, "other Task");
    OS_CreateTask(DisplayTask, NULL, NULL, Display_TASK_STACK_SIZE, Display_TASK_PRIORITY, 0, 0, "display Task");
        
    while(1)
    {
        if(OS_WaitEvent(mainTaskHandle, (void **)&event, OS_TIME_OUT_WAIT_FOREVER))
        {
            EventDispatch(event);
            OS_Free(event->pParam1);
            OS_Free(event->pParam2);
            OS_Free(event);
        }
    }
}

void app_Main(void)
{
    mainTaskHandle = OS_CreateTask(AppMainTask ,
        NULL, NULL, AppMain_TASK_STACK_SIZE, AppMain_TASK_PRIORITY, 0, 0, "init Task");
    OS_SetUserMainHandle(&mainTaskHandle);
}
