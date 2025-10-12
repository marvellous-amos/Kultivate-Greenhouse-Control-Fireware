#ifndef LOGGER_H
#define LOGGER_H

#include <Firebase_ESP_Client.h>

void setupTime();
void logSensorData(float temp, float hum, int soil, int gas);

#endif