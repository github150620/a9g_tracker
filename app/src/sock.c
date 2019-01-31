/*
 *
 */
#include <errno.h>

#include "api_debug.h"
#include "api_socket.h"

#include "log.h"

#define Socket_GetLastError         CSDK_FUNC(Socket_GetLastError)

char sock_host[64];
int sock_port;

int sock_fd = -1;

void sock_init(char* domain, int port) {
    memcpy(sock_host, domain, strlen(domain));
    sock_port = port;
}

int sock_status() {
    if (sock_fd == -1) {
        return 0;
    }
    return 1;
}

bool sock_connect() {
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

    /*
    int flags = fcntl(sock_fd, F_GETFL, 0);
    if (flags < 0) {
        log_print("fcntl(F_GETFL)...<0");
        close(sock_fd);
        return false;
    }

    flags |= O_NONBLOCK;
    if (fcntl(sock_fd, F_SETFL, flags) <0) {
        log_print("fcntl(F_SETFL)...<0");
        close(sock_fd);
        return false;
    }
    */

    log_print("connect()");
    int ret = connect(sock_fd, (struct sockaddr*)&sockaddr, sizeof(struct sockaddr_in));
    if(ret < 0){
        log_print("connect()...-1");
        close(sock_fd);
        sock_fd = -1;
        return false;
    }
    return true;
}

bool sock_request(char* data, int dataLen, char* responseBuf, int bufLen) {
    if (sock_fd == -1) {
        return false;
    }

    log_print("send");
    int ret = send(sock_fd, data, dataLen, 0);
    if (ret < 0) {
        log_print("send()...-1");
        close(sock_fd);
        sock_fd = -1;
        return false;
    }

    struct fd_set fds;
    struct timeval timeout={30,0};
    FD_ZERO(&fds);
    FD_SET(sock_fd,&fds);
    log_print("select()");
    ret = select(sock_fd+1,&fds,NULL,NULL,&timeout);
    switch (ret) {
    case -1:
        log_print("select()...-1");
        close(sock_fd);
        sock_fd = -1;
        return false;
    case 0:
        log_print("select()...0");
        close(sock_fd);
        sock_fd = -1;
        return false;
    default:
        if(FD_ISSET(sock_fd,&fds))
        {
            memset(responseBuf,0,bufLen);
            ret = recv(sock_fd,responseBuf,bufLen,0);
            if(ret <= 0) {
                Trace(1, "recv()<=0");
                close(sock_fd);
                sock_fd = -1;
                return false;
            }
            responseBuf[ret] = '\0';
        }
        break;
    }
    return true;
}
