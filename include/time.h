#ifndef _EM_TIME_H
#define _EM_TIME_H
#include<stdlib.h>
extern "C" {
void startRTC();
long getCurrentSecondsSince2000();
int timeToISOString(char* buf, size_t bufSize, long time);
}

#endif
