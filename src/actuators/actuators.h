#ifndef ACTUATORS_H
#define ACTUATORS_H

#include <Arduino.h>

#define PUMP_RELAY_PIN        16   // Water pump
#define EXHAUST_FAN_RELAY_PIN 27   // Exhaust fan
#define CIRC_FAN_RELAY_PIN    32   // Circulation fan
// #define FOGGER_RELAY_PIN      14   // Fogger (optional)

// #define RELAY_PIN_1 14  // GPIO16
// #define RELAY_PIN_2 16  // GPIO27
// #define RELAY_PIN_3 27  // GPIO14
// #define RELAY_PIN_4 32  // GPIO32
#define RELAY_ON    LOW
#define RELAY_OFF   HIGH

// Function prototypes
void setupActuators();

void controlActuators(int soilMoisture, int gasLevel, float temperature, float humidity);


// Helpers to control individual actuators
void pumpOn();
void pumpOff();

void exhaustFanOn();
void exhaustFanOff();

void circulationFanOn();
void circulationFanOff();


#endif
