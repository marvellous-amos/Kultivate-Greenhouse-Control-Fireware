/*************************************************************
 *  ActuatorControl.cpp
 *  Module: Actuator Control for Pepper Cultivator
 *
 *  Description:
 *    Handles automation logic and relay control for irrigation,
 *    ventilation, and humidity regulation.
 *************************************************************/

#include "actuators/actuators.h"

// -----------------------------
// Control Thresholds (adjust after calibration)
// -----------------------------
const int DRY_SOIL_THRESHOLD = 2500;  // Example: dry soil
const int WET_SOIL_THRESHOLD = 1400;  // Example: wet soil
const float MAX_TEMP = 32.0;          // °C
const float MIN_TEMP = 22.0;          // °C
const float MAX_HUMIDITY = 85.0;      // %
const float MIN_HUMIDITY = 60.0;      // %


// -----------------------------
// Function: setupActuators()
// -----------------------------
void setupActuators() {
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  pinMode(EXHAUST_FAN_RELAY_PIN, OUTPUT);
  pinMode(CIRC_FAN_RELAY_PIN, OUTPUT);
//   pinMode(FOGGER_RELAY_PIN, OUTPUT);

  // Initialize all OFF (HIGH = inactive for most relay modules)
  digitalWrite(PUMP_RELAY_PIN, HIGH);
  digitalWrite(EXHAUST_FAN_RELAY_PIN, HIGH);
  digitalWrite(CIRC_FAN_RELAY_PIN, HIGH);
//   digitalWrite(FOGGER_RELAY_PIN, HIGH);

  Serial.println("[INIT] Actuators ready.");
}


// -----------------------------
// Function: controlActuators()
// Purpose: React to soil & climate sensor readings
// -----------------------------
void controlActuators(int soilMoistureValue, float temperature, float humidity) {
  Serial.println("=== Actuator Control Loop ===");

  // --- Soil Moisture Control ---
  if (soilMoistureValue > DRY_SOIL_THRESHOLD) {
    pumpOn();
  } else if (soilMoistureValue < WET_SOIL_THRESHOLD) {
    pumpOff();
  }

  // --- Temperature Control ---
  if (temperature > MAX_TEMP) {
    exhaustFanOn();
  } else if (temperature < MIN_TEMP) {
    exhaustFanOff();
  }

  // --- Humidity Control ---
  if (humidity < MIN_HUMIDITY) {
    circulationFanOn();
  } else if (humidity > MAX_HUMIDITY) {
    circulationFanOff();
  }

  // --- Circulation fan: always on ---
//   circulationFanOn();

  Serial.println("=============================");
}


// -----------------------------
// Helper Functions (Relay Control)
// -----------------------------
void pumpOn()             { digitalWrite(PUMP_RELAY_PIN, LOW);  Serial.println("Pump: ON"); }
void pumpOff()            { digitalWrite(PUMP_RELAY_PIN, HIGH); Serial.println("Pump: OFF"); }

void exhaustFanOn()       { digitalWrite(EXHAUST_FAN_RELAY_PIN, LOW);  Serial.println("Exhaust Fan: ON"); }
void exhaustFanOff()      { digitalWrite(EXHAUST_FAN_RELAY_PIN, HIGH); Serial.println("Exhaust Fan: OFF"); }

void circulationFanOn()   { digitalWrite(CIRC_FAN_RELAY_PIN, LOW);  Serial.println("Circulation Fan: ON"); }
void circulationFanOff()  { digitalWrite(CIRC_FAN_RELAY_PIN, HIGH); Serial.println("Circulation Fan: OFF"); }

// void foggerOn()           { digitalWrite(FOGGER_RELAY_PIN, LOW);  Serial.println("Fogger: ON"); }
// void foggerOff()          { digitalWrite(FOGGER_RELAY_PIN, HIGH); Serial.println("Fogger: OFF"); }
