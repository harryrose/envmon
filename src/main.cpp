#include <Arduino.h>
#include<WiFi.h>
#include<HTTPClient.h>

#include <stdlib.h>
#include "config.h"
#include "time.h"
#include "readings.h"
#include "eeprom.hpp"
#include "store.h"

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define ISO_STRING_SIZE 32

void sendData();
void setup();

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR unsigned short eepromOffset = 0;


void sendData() {
  IPAddress local_IP(LOCAL_IP);
  IPAddress gateway(GATEWAY_IP);
  IPAddress subnet(SUBNET_MASK);
  IPAddress primaryDNS(DNS_PRIMARY); 
  IPAddress secondaryDNS(DNS_SECONDARY);
  unsigned short offset = 0;
  t_reading reading;
 
  digitalWrite(LED_PIN,LOW);
  Serial.println("waiting for wifi");
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
 
  digitalWrite(LED_PIN,HIGH);

  while((WiFi.status() != WL_CONNECTED)) {
    digitalWrite(LED_PIN,HIGH);
    delay(100);
    digitalWrite(LED_PIN,LOW);
    delay(50);
  }
  Serial.println("wifi established");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  Thingspeak ts(THINGSPEAK_CHANNEL, THINGSPEAK_KEY);
  if(!ts.Begin()) {
    Serial.println("Did not start ts");
  } else {
    for(int i = 0; i < BUFFER_POINTS; i++) {
      offset = readFromEEProm<t_reading>(reading,offset);
      if(!ts.Store(reading)){
        Serial.println("Did not store reading");
      }
    }
    if(!ts.End()){
      Serial.println("Did not end ts");
    }
  }
  
  digitalWrite(LED_PIN,LOW);
  WiFi.disconnect(true);
  WiFi.persistent(false); 
}

void setup() {
  delay(500);
  Serial.begin(9600);
  RTCBegin();
  EEPromBegin();

  pinMode(LED_PIN,OUTPUT);
  delay(500);
  bootCount ++;
  Serial.print("Boot ");
  Serial.println(bootCount, DEC);
  t_reading reading;
  int state = obtainReading(reading);
  if(state == RR_OK) {
    Serial.println("OK");
    eepromOffset = writeToEEProm<t_reading>(reading,eepromOffset);
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
