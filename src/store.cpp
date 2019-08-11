#include "store.h"
#include "time.h"
#include <string>
#include <stdlib.h>

using namespace std;

Thingspeak::Thingspeak(string channel_id, string write_key) {
    this->channel_id = channel_id;
    this->write_key = write_key;
}

inline bool writeChunk(WiFiClient *client, size_t size, char *buf) {
    char tmpBuf[10];
    size_t written;
    written = snprintf(tmpBuf, 10, "%X\r\n",size);
    Serial.print(tmpBuf);
    if(written != client->write(tmpBuf, written)) {
        return false;
    }
    Serial.print(buf);
    if(size != client->write(buf,size)) {
        return false;
    }
    Serial.print("\r\n");
    if(2 != client->write("\r\n",2)) {
        return false;
    }
    return true;
}

bool Thingspeak::Begin() {
    size_t written;
    this->first = true;
    IPAddress ip(34,226,171,107);
    if(!this->client.connect(ip,80)){
        return false;
    }
    written = snprintf(this->buf, TS_STORE_BUF_SIZE,"POST /channels/%s/bulk_update.json HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nTransfer-Encoding: chunked\r\nContent-Type: application/json\r\n\r\n",this->channel_id.c_str(),ip.toString().c_str());
    Serial.print(this->buf);
    if(written != client.write(this->buf, written)) {
        return false;
    }
    written = snprintf(this->buf, TS_STORE_BUF_SIZE,"{\"write_api_key\":\"%s\",\"updates\":[", this->write_key.c_str());
    return writeChunk(&client, written, this->buf);
}

bool Thingspeak::Store(t_reading &reading) {
    size_t written;
    char isoString[32];
    char comma[2] = {'\0','\0'};
    if(!this->first) {
        comma[0] = ',';
    }
    this->first = false;

    timeToISOString(isoString, 32, reading.secondsTime);
    written = snprintf(this->buf, TS_STORE_BUF_SIZE, "%s{\"created_at\":\"%s\",\"field1\":%4.2f,\"field2\":%4.2f,\"field3\":%4.2f,\"field4\":%7.2f,\"field5\":%5.2f,\"field6\":%7.0f}",comma,isoString, reading.solar * 0.0019, reading.batt * 0.0019, reading.temperature, reading.humidity, reading.light, reading.pressure);
    return writeChunk(&client, written, this->buf);
}

bool Thingspeak::End() {
    size_t written = snprintf(this->buf,TS_STORE_BUF_SIZE,"]}");
    writeChunk(&client, written, this->buf);
    writeChunk(&client,0,this->buf);
    client.write("\r\n", 2);
    Serial.print("\r\n");

    this->client.stop();
    return true;
}