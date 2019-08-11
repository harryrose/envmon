#include <Arduino.h>
#include<WiFi.h>
#include<HTTPClient.h>
#include <EEPROM.h>
#include <stdlib.h>
#include "config.h"
#include "time.h"
#include "readings.h"

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define ISO_STRING_SIZE 32

unsigned short writeToEEProm(t_reading &reading, unsigned short startAddress);
unsigned short readFromEEProm(t_reading &reading, unsigned short startAddress);
void sendData();
void setup();

unsigned short writeToEEProm(t_reading &reading, unsigned short startAddress){
  const byte *bytes = (const byte*)(void*)&reading;
  for(unsigned char i = 0; i < sizeof(t_reading); i++, startAddress++, bytes++) {
    EEPROM.write(startAddress, *bytes);
  }
  EEPROM.commit();
  return startAddress;
}


unsigned short readFromEEProm(t_reading &reading, unsigned short startAddress) {
  byte *bytes = (byte *)(void *)&reading;
  for(unsigned char i = 0; i < sizeof(t_reading); i++, startAddress++, bytes++) {
    *bytes = EEPROM.read(startAddress);
  }
  return startAddress;
}

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR unsigned short eepromOffset = 0;


void sendData() {
  digitalWrite(LED_PIN,LOW);
  IPAddress local_IP(LOCAL_IP);
  IPAddress gateway(GATEWAY_IP);
  
  // Following three settings are optional
  IPAddress subnet(SUBNET_MASK);
  IPAddress primaryDNS(DNS_PRIMARY); 
  IPAddress secondaryDNS(DNS_SECONDARY);
  
 
  unsigned short offset = 0;
  char isoString[ISO_STRING_SIZE];
  char postBuf[51 + 136 * BUFFER_POINTS];
  t_reading reading;
  Serial.println("waiting for wifi");
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
 
  digitalWrite(LED_PIN,HIGH);
  int payloadSize = 0;
  int amountWritten = 0;
  char * writeHead = postBuf;
  
  amountWritten = sprintf(writeHead, "{\"write_api_key\":\"" THINGSPEAK_KEY "\",\"updates\":[");
  writeHead += amountWritten;
  payloadSize += amountWritten;
  
  HTTPClient http;
  
  for(int i = 0; i < BUFFER_POINTS; i++) {
    offset = readFromEEProm(reading,offset);
    if(i != 0) {
      amountWritten = sprintf(writeHead, ",");
      writeHead += amountWritten;
      payloadSize += amountWritten;
    }
    timeToISOString(isoString, ISO_STRING_SIZE, reading.secondsTime);
    
    amountWritten = sprintf(writeHead, "{\"created_at\":\"%s\",\"field1\":%4.2f,\"field2\":%4.2f,\"field3\":%4.2f,\"field4\":%7.2f,\"field5\":%5.2f,\"field6\":%7.0f}",isoString, reading.solar * 0.0019, reading.batt * 0.0019, reading.temperature, reading.humidity, reading.light, reading.pressure);
    writeHead += amountWritten;
    payloadSize += amountWritten;
  }
  amountWritten = sprintf(writeHead, "]}");
  writeHead += amountWritten;
  payloadSize += amountWritten;
  
  String toSend(postBuf);
  Serial.println(toSend);
   while((WiFi.status() != WL_CONNECTED)) {
    digitalWrite(LED_PIN,HIGH);
    delay(100);
    digitalWrite(LED_PIN,LOW);
    delay(50);
  }
  http.begin("http://34.226.171.107/channels/" THINGSPEAK_CHANNEL "/bulk_update.json");
  http.addHeader("Content-Type","application/json");
  int status = http.POST(toSend);
  Serial.print("Status: ");
  Serial.print(payloadSize, DEC);
  Serial.println(status, DEC);
  String str = http.getString();
  Serial.println(str);
  http.end();

  digitalWrite(LED_PIN,LOW);
  WiFi.disconnect(true); // delete old config
  WiFi.persistent(false); 
}
void setup() {
  delay(500);

  EEPROM.begin(512);
  Serial.begin(9600);
  startRTC();
  
  pinMode(LED_PIN,OUTPUT);
  delay(500);
  bootCount ++;
  Serial.print("Boot ");
  Serial.println(bootCount, DEC);
  t_reading reading;
  int state = obtainReading(reading);
  if(state == RR_OK) {
    Serial.println("OK");
    eepromOffset = writeToEEProm(reading,eepromOffset);
    Serial.print("Solar: ");
    Serial.print(reading.solar,DEC);
    Serial.print("\nBatt: ");
    Serial.print(reading.batt,DEC);
    Serial.print("\nTemp: ");
    Serial.print(reading.temperature, 2);
    Serial.print("\nHum: ");
    Serial.print(reading.humidity, 2);
    Serial.print("\nPressure: ");
    Serial.print(reading.pressure, 2);
    Serial.print("\nLight: ");
    Serial.print(reading.light, 2);
    Serial.print("\nSize: ");
    Serial.print(sizeof(t_reading), DEC);
    Serial.println();
    
    digitalWrite(LED_PIN,HIGH);
    delay(50);
    digitalWrite(LED_PIN,LOW);
    if(bootCount == BUFFER_POINTS){
      Serial.println("Would send buffer");
      sendData();
      eepromOffset = 0;
      bootCount = 0;
    }
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  } else {
    digitalWrite(LED_PIN,HIGH);
    delay(50);
    digitalWrite(LED_PIN,LOW);
    delay(50);
    digitalWrite(LED_PIN,HIGH);
    delay(50);
    digitalWrite(LED_PIN,LOW);
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP_AFTER_FAILURE * uS_TO_S_FACTOR);
  }
  Serial.println("Sleeping");

  
  esp_deep_sleep_start();
}

void loop() {

}
