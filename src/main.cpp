#include <Arduino.h>
#include "config.h"

#include <Adafruit_Si7021.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_TSL2561_U.h>

#include<WiFi.h>
#include<HTTPClient.h>

#include "RTClib.h"
#include <EEPROM.h>
#include <stdlib.h>

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */

typedef struct t_reading{
  long secondsTime;
  int solar;
  int batt;
  float humidity;
  float temperature;
  float pressure;
  float light;
} t_reading;

#define OK 1
#define NOK 0

int obtainSIReading(float &tempReading, float &humReading);
int obtainBMEReading(float &reading);
int obtainTSLReading(float &reading);
int sampleRaw(int pin);
int obtainReading(t_reading &reading);

unsigned short writeToEEProm(t_reading &reading, unsigned short startAddress);
unsigned short readFromEEProm(t_reading &reading, unsigned short startAddress);
void sendData();
void setup();

Adafruit_Si7021 hum = Adafruit_Si7021();
Adafruit_BMP280 bme;
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
RTC_DS1307 RTC;

int obtainSIReading(float &tempReading, float &humReading) {
  if (!hum.begin()) {
    Serial.println("Did not find Si7021 sensor!");
    return NOK;
  }
  tempReading = hum.readTemperature();
  humReading = hum.readHumidity();

  return OK;
}

int obtainBMEReading(float &reading) {
  if (!bme.begin(0x76)) {  
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    return NOK;
  }
  
  reading = bme.readPressure();
  return OK;
}

int obtainTSLReading(float &reading) {
  sensors_event_t event;
  if(!tsl.begin())
  {
    /* There was a problem detecting the TSL2561 ... check your connections */
    Serial.print("Ooops, no TSL2561 detected ... Check your wiring or I2C ADDR!");
    return NOK;
  }
  tsl.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);
   
  tsl.getEvent(&event);
  reading = event.light;
  return OK;
}

int sampleShift = 3;
int sampleRaw(int pin) {
  int out = 0;
  for(int i = 0; i < (1 << sampleShift); i ++) {
    out += analogRead(pin);
  }
  return out >> sampleShift;
}


int obtainReading(t_reading &reading) {
  int state = obtainTSLReading(reading.light);
  if(state != OK){
    return state;
  }
  
  state = obtainBMEReading(reading.pressure);
  if(state != OK){
    return state;
  }
  
  state = obtainSIReading(reading.temperature, reading.humidity);
  if(state != OK){
    return state;
  }
  
  reading.solar = sampleRaw(SOLAR_PIN);
  reading.batt = sampleRaw(BATT_PIN);
  reading.secondsTime = RTC.now().secondstime();
  return OK;
}

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
  char isoString[32];
  char buf[256];
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
    DateTime dt(reading.secondsTime);
    if(i != 0) {
      amountWritten = sprintf(writeHead, ",");
      writeHead += amountWritten;
      payloadSize += amountWritten;
    }
    sprintf(isoString, "%04d-%02d-%02d %02d:%02d:%02d +0100",dt.year(),dt.month(),dt.day(),dt.hour(),dt.minute(),dt.second());
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
  RTC.begin();
  pinMode(LED_PIN,OUTPUT);
  delay(500);
  bootCount ++;
  Serial.print("Boot ");
  Serial.println(bootCount, DEC);
  t_reading reading;
  int state = obtainReading(reading);
  if(state == OK) {
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
