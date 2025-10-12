#include "sensors.h"

DHT dht1(DHT_PIN_1, DHT_TYPE);
DHT dht2(DHT_PIN_2, DHT_TYPE);

void initSensors() {
  dht1.begin();
  dht2.begin();
  analogReadResolution(12);
}

DHTReadings readDHTSensors() {
  DHTReadings r;
  r.humidity1 = dht1.readHumidity();
  r.temperature1 = dht1.readTemperature();
  r.humidity2 = dht2.readHumidity();
  r.temperature2 = dht2.readTemperature();
  return r;
}

int readSoilSensor() {
  int val = analogRead(soilSensorPin);
  Serial.printf("Soil moisture ADC: %d\n", val);
  return val;
}

int readGasSensor() {
  int val = analogRead(mqPin);
  Serial.printf("Gas sensor ADC: %d\n", val);
  return val;
}
