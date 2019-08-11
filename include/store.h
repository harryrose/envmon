#ifndef _EM_STORE_H_
#define _EM_STORE_H_
#include "readings.h"
#include <WiFi.h>
#include <string>
#define TS_STORE_BUF_SIZE 512

using namespace std;

class DataStore {
    public:
    virtual bool Begin() =0;
    virtual bool Store(t_reading &reading) =0;
    virtual bool End() =0;
};

class Thingspeak : public DataStore {
    public:
    Thingspeak(string channel_id, string write_key);

    bool Begin();
    bool Store(t_reading &reading);
    bool End();

    private:
    bool first;
    string channel_id;
    string write_key;
    WiFiClient client;
    char buf[TS_STORE_BUF_SIZE];
};

#endif
