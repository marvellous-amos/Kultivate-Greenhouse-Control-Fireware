#ifndef SENSORS_H
#define SENSORS_H

#include <DHT.h>

#define DHT_PIN_1 4
#define DHT_PIN_2 15
#define DHT_TYPE DHT22
#define soilSensorPin 35
#define mqPin 34
#define wetSoil 1600
#define drySoil 3000

struct DHTReadings {
  float temperature1;
  float humidity1;
  float temperature2;
  float humidity2;
};

void initSensors();
DHTReadings readDHTSensors();
int readSoilSensor();
int readGasSensor();

#endif
