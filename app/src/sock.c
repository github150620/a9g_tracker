/*
 *
 */
#include "api_debug.h"
#include "api_os.h"
#include "api_socket.h"

#include "log.h"

#define SOCK_RECV_TASK_SIZE      (1024 * 4)
#define SOCK_RECV_TASK_PRIORITY  16

#define SOCK_SEND_TASK_SIZE      (1024 * 4)
#define SOCK_SEND_TASK_PRIORITY  17

#define SOCK_QUEUE_SIZE 32

char sock_host[64];
int sock_port;
char sock_firstSend[128];
int sock_isConnected = 0;

HANDLE sock_bufMutex = NULL;

int sock_fd = -1;

int sock_sendBufIndex1 = 0;
int sock_sendBufIndex2 = 0;
char sock_sendBufQueue[SOCK_QUEUE_SIZE][128];

bool SOCK_Connect() {
    sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock_fd < 0){
        Trace(1, "socket()<0");
        return false;
    }

    uint8_t ip[16];
    if(DNS_GetHostByName2(sock_host,ip) != 0)
    {
        Trace(1, "DNS_GetHostByName2()...!0");
        close(sock_fd);
        sock_fd = -1;
        return false;
    }

    struct sockaddr_in sockaddr;
    memset(&sockaddr,0,sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(sock_port);
    inet_pton(AF_INET, (char*)ip, &sockaddr.sin_addr);

    int ret = connect(sock_fd, (struct sockaddr*)&sockaddr, sizeof(struct sockaddr_in));
    if(ret < 0){
        Trace(1, "connect()...-1");
        close(sock_fd);
        sock_fd = -1;
        return false;
    }
    sock_isConnected = 1;

    int len = send(sock_fd, sock_firstSend, strlen(sock_firstSend), 0);    
    if (len<=0) {
        close(sock_fd);
        sock_fd = -1;
        return false;
    }
    return true;
}

void SOCK_WriteBuf(char* data) {
    int len;
    OS_LockMutex(sock_bufMutex);
    len = strlen(data);
    memcpy(sock_sendBufQueue[sock_sendBufIndex2], data, len);
    sock_sendBufQueue[sock_sendBufIndex2][len] = '\0';
    sock_sendBufIndex2++;
    sock_sendBufIndex2 %= SOCK_QUEUE_SIZE;
    if (sock_sendBufIndex2==sock_sendBufIndex1) {
        sock_sendBufIndex1++;
        sock_sendBufIndex1 %= SOCK_QUEUE_SIZE;
    }
    OS_UnlockMutex(sock_bufMutex);
}

int SOCK_Status() {
    return sock_isConnected;
}

void SOCK_RecvTask(VOID *pData) {
    int len;
    int ret;
    char rbuf[128];

    struct fd_set fds;
    struct timeval timeout;

    while (1) {
        if (sock_fd==-1) {
            if (!SOCK_Connect()) {
                Trace(1, "SOCK_Connect()...failed");
                OS_Sleep(2000);
                continue;
            }
        }

        FD_ZERO(&fds);
        FD_SET(sock_fd,&fds);
        timeout.tv_sec = 120;
        timeout.tv_usec = 0;
        ret = select(sock_fd+1,&fds,NULL,NULL,&timeout);
        switch (ret) {
        case -1:
            close(sock_fd);
            sock_isConnected = 0;
            sock_fd = -1;
            continue;
        case 0:
            close(sock_fd);
            sock_isConnected = 0;
            sock_fd = -1;
            continue;
        default:
            if(FD_ISSET(sock_fd,&fds))
            {
                len = recv(sock_fd, rbuf, sizeof(rbuf), 0);
                switch (len) {
                case -1:
                    Trace(1, "recv()...-1");
                    close(sock_fd);
                    sock_isConnected = 0;
                    sock_fd = -1;
                    OS_Sleep(1000);
                    continue;
                case 0:
                    Trace(1, "recv()...0");
                    close(sock_fd);
                    sock_isConnected = 0;                    
                    sock_fd = -1;
                    OS_Sleep(1000);
                    continue;
                default:
                    rbuf[len] = '\0';
                    Trace(1, "SOCK recv:%s", rbuf);
                    continue;
                }
            }
            break;
        }
    }
}

void SOCK_SendTask(VOID *pData) {
    char wbuf[128];

    int index1;
    int index2;
    while(1) {
        if (sock_isConnected==0) {
            OS_Sleep(1000);
            continue;
        }        
        OS_LockMutex(sock_bufMutex);
        index1 = sock_sendBufIndex1;
        index2 = sock_sendBufIndex2;
        if ( index1 != index2 ) {
            memcpy(wbuf, sock_sendBufQueue[index1], strlen(sock_sendBufQueue[index1])+1);
        }
        OS_UnlockMutex(sock_bufMutex);
        if ( index1 != index2 ) {

            int ret = send(sock_fd, wbuf, strlen(wbuf), 0);
            if (ret>0) {
                OS_LockMutex(sock_bufMutex);                    
                sock_sendBufIndex1++;
                sock_sendBufIndex1 %= SOCK_QUEUE_SIZE;
                OS_UnlockMutex(sock_bufMutex);
            } else {
                OS_Sleep(5000);
            }
        } else {
            OS_Sleep(1000);
        }
    }
}



void SOCK_Init(char* domain, int port, char* firstSend) {
    memset(sock_sendBufQueue, 0, sizeof(sock_sendBufQueue));

    sock_bufMutex = OS_CreateMutex();

    memcpy(sock_host, domain, strlen(domain));
    sock_port = port;
    memcpy(sock_firstSend, firstSend, strlen(firstSend));

    OS_CreateTask(SOCK_RecvTask, NULL, NULL, SOCK_RECV_TASK_SIZE, SOCK_RECV_TASK_PRIORITY, 0, 0, "SOCK recv task");
    OS_CreateTask(SOCK_SendTask, NULL, NULL, SOCK_SEND_TASK_SIZE, SOCK_SEND_TASK_PRIORITY, 0, 0, "SOCK send task");
}
