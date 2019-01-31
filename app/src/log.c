//


#include <stdio.h>
#include <string.h>
#include <time.h>

#include "log.h"

#include "api_debug.h"
#include "api_fs.h"

int32_t log_fd = -1;
char log_path[32];

// The path should begin with "/t/".
bool log_init(char* path) {
    strcpy(log_path, path);
    log_fd = API_FS_Open(log_path, FS_O_RDWR | FS_O_CREAT | FS_O_APPEND, 0);
 	if ( log_fd < 0)
	{
        Trace(1,"API_FS_Open()...%d", log_fd);
		return false;
	}
    log_print("--------");
    return true;
}

bool log_print(char* content) {

    if (log_fd == -1) {
        return false;
    }

    int32_t ret;

    char buf[24];
    RTC_Time_t t;
    TIME_GetRtcTIme(&t);
    sprintf(buf, "\r\n%04d-%02d-%02d %02d:%02d:%02d ", t.year, t.month, t.day, t.hour, t.minute, t.second);
    ret = API_FS_Write(log_fd, (uint8_t*)buf, strlen(buf));
    if ( ret <= 0 ) {
        Trace(1, "API_FS_Write()...%d", ret);
        return false;
    }

    int n = strlen(content);
    ret = API_FS_Write(log_fd, (uint8_t*)content, (n>128)?128:n);
    if ( ret <= 0 ) {
        Trace(1, "API_FS_Write()...%d", ret);
        return false;
    }

    API_FS_Flush(log_fd);
    return true;
}
