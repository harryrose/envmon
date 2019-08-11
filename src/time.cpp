#include "time.h"
#include "RTClib.h"

RTC_DS1307 RTC;

extern "C" {
void startRTC() {
    RTC.begin();
}

long getCurrentSecondsSince2000() {
    return RTC.now().secondstime();
}

int timeToISOString(char* buf, size_t bufSize, long time){
    DateTime dt(time);
    return snprintf(buf, bufSize, "%04d-%02d-%02d %02d:%02d:%02d +0000",dt.year(),dt.month(),dt.day(),dt.hour(),dt.minute(),dt.second());
}
}
