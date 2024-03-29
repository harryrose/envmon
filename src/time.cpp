#include "time.h"
#include "RTClib.h"
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

RTC_DS1307 RTC;

// having to use C naming here because of reasons I don't yet understand...
// Linker isnt finding C++ obfuscated symbols for this particular file, although
// other files are working fine.
extern "C" {
    void RTCBegin() {
        RTC.begin();
    }

    long getCurrentSecondsSince2000() {
        return RTC.now().secondstime();
    }

    int timeToISOString(char* buf, size_t bufSize, long time){
        DateTime dt(time);
        return snprintf(buf, bufSize, "%04d-%02d-%02d %02d:%02d:%02d +0000",dt.year(),dt.month(),dt.day(),dt.hour(),dt.minute(),dt.second());
    }

    void SyncTime() {
        if(!RTC.isrunning()) {
            RTC.begin();
        }
        Serial.print("Updating time: Current - ");
        Serial.println(RTC.now().timestamp());

        WiFiUDP ntpUDP;
        NTPClient timeClient(ntpUDP);
        timeClient.begin();
        while(!timeClient.update()) {
            timeClient.forceUpdate();
        }
        
        DateTime dt(timeClient.getEpochTime() - SECONDS_FROM_1970_TO_2000);
        Serial.print("Updating time: New - ");
        Serial.println(dt.timestamp());
        RTC.adjust(dt);
    }
}
