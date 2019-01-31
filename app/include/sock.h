
#ifndef __SOCK_H__
#define __SOCK_H__

void sock_init(char* ip, int port);
bool sock_connect();
bool sock_request(char* data, int dataLen, char* responseBuf, int bufLen);
int  sock_status();

#endif
