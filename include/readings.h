#ifndef _EM_READINGS_H_
#define _EM_READINGS_H_

typedef enum ReadingResult {
  RR_NOK = 0,
  RR_OK = 1
} ReadingResult;

// t_reading Stores a timestamped measurement from the various sensors attached to the microcontroller
typedef struct t_reading{
  // secondsTime is the number of seconds since 2000-01-01T00:00:00Z
  long secondsTime;
  // solar holds raw ADC values measuring voltage across the solar panel
  int solar;
  // batt holds raw ADC values measuring voltage across the battery terminals
  int batt;
  // humidity is a floating point percentage relative humidity
  float humidity;
  // temperature is a floating point measurement of the current temperature in degrees centigrade
  float temperature;
  // pressure is a floating point measurement of the current pressure in Pascals
  float pressure;
  // light is a floating point measurement of the current light intensity in lux
  float light;
} t_reading;

ReadingResult obtainReading(t_reading &reading);

#endif
