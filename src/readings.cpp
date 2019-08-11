#include "readings.h"
#include "time.h"
#include "config.h"

#include <Adafruit_Si7021.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_TSL2561_U.h>

Adafruit_Si7021 hum = Adafruit_Si7021();
Adafruit_BMP280 bme;
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

ReadingResult obtainSIReading(float &tempReading, float &humReading);
ReadingResult obtainBMEReading(float &reading);
ReadingResult obtainTSLReading(float &reading);
int sampleRaw(int pin);

ReadingResult obtainSIReading(float &tempReading, float &humReading) {
  if (!hum.begin()) {
    Serial.println("Did not find Si7021 sensor!");
    return RR_NOK;
  }
  tempReading = hum.readTemperature();
  humReading = hum.readHumidity();

  return RR_OK;
}

ReadingResult obtainBMEReading(float &reading) {
  if (!bme.begin(0x76)) {  
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    return RR_NOK;
  }
  
  reading = bme.readPressure();
  return RR_OK;
}

ReadingResult obtainTSLReading(float &reading) {
  sensors_event_t event;
  if(!tsl.begin())
  {
    /* There was a problem detecting the TSL2561 ... check your connections */
    Serial.print("Ooops, no TSL2561 detected ... Check your wiring or I2C ADDR!");
    return RR_NOK;
  }
  tsl.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);
   
  tsl.getEvent(&event);
  reading = event.light;
  return RR_OK;
}

int sampleShift = 3;
int sampleRaw(int pin) {
  int out = 0;
  for(int i = 0; i < (1 << sampleShift); i ++) {
    out += analogRead(pin);
  }
  return out >> sampleShift;
}


ReadingResult obtainReading(t_reading &reading) {
  ReadingResult state = obtainTSLReading(reading.light);
  if(state != RR_OK){
    return state;
  }
  
  state = obtainBMEReading(reading.pressure);
  if(state != RR_OK){
    return state;
  }
  
  state = obtainSIReading(reading.temperature, reading.humidity);
  if(state != RR_OK){
    return state;
  }
  
  reading.solar = sampleRaw(SOLAR_PIN);
  reading.batt = sampleRaw(BATT_PIN);
  reading.secondsTime = getCurrentSecondsSince2000();
  return RR_OK;
}
