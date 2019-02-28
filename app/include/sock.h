
#ifndef __SOCK_H__
#define __SOCK_H__

void SOCK_Init(char* domain, int port, char* firstSend);
void SOCK_WriteBuf(char* data);
int  SOCK_Status();

#endif
