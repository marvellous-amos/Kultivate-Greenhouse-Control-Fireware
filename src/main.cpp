/*
 * Kultivate Greenhouse Control Firmware
 * ESP32 with Firebase Integration
 * Supports Normal Mode and Simulation Mode
 *
 * Changes:
 *  - Replaced ambiguous String '+' usage with concat() via buildPath()
 *  - Removed anonymous Firebase sign-up flow (no auth required)
 *  - signupOK set true after Firebase.begin()
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <DHT.h>

// WiFi credentials
#define WIFI_SSID     "Oracle"
#define WIFI_PASSWORD "12345678"

// Firebase credentials (no authentication required)
#define API_KEY       "AIzaSyDN3eUAOAWhFBsrez4UrgtEKjfAAv57Y3g"
#define DATABASE_URL  "https://evidence-3abac-default-rtdb.firebaseio.com/"
#define DATABASE_ST   "jas65T0JYHiger39Z9jcWpIVELnmPF4MUnWf7xTX"


// Sensor Pins
#define DHT_PIN 4
#define DHT_TYPE DHT22
#define DHT_PIN_2 15
#define SOIL_MOISTURE_PIN 35
#define GAS_SENSOR_PIN 34
#define LIGHT_SENSOR_PIN 32

// Actuator Pins
#define FAN_PIN 32
#define FAN_PIN_2 27
#define PUMP_PIN 16
#define HEATER_PIN 27
#define MISTING_PIN 14
#define LIGHTING_PIN 12
#define CO2_DOSING_PIN 13

// LED Indicator
#define STATUS_LED 2

// Zone ID - Configure for each greenhouse
#define ZONE_ID "greenhouse_1"

// Timing
#define SENSOR_READ_INTERVAL 5000    // 5 seconds
#define FIREBASE_UPDATE_INTERVAL 10000  // 10 seconds
#define SIMULATION_CHECK_INTERVAL 1000  // 1 second
unsigned long lastModeChangeTime = 0;
#define MODE_CHANGE_COOLDOWN 5000 // 5 seconds


// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;      // kept for compatibility with Firebase.begin signature
FirebaseConfig config;

// Sensor objects
DHT dht(DHT_PIN, DHT_TYPE);

// System state
enum SystemMode {
  NORMAL_MODE,
  SIMULATION_MODE
};

SystemMode currentMode = NORMAL_MODE;
bool signupOK = false;

// Sensor readings
struct SensorData {
  float temperature;
  float humidity;
  float soilMoisture;
  float gasLevel;
  float lightLevel;
};

SensorData currentSensors = {0, 0, 0, 0, 0};

// Normal mode parameters
struct NormalModeParams {
  float targetTempMin = 22.0;
  float targetTempMax = 28.0;
  float targetHumidityMin = 60.0;
  float targetHumidityMax = 80.0;
  float targetSoilMoistureMin = 40.0;
  float targetSoilMoistureMax = 70.0;
  float targetGasLevelMin = 400.0;
  float targetLightLevelMin = 200.0;
};

NormalModeParams normalParams;

// Actuator states
struct ActuatorStates {
  bool fan = false;
  bool pump = false;
  bool heater = false;
  bool misting = false;
  bool lighting = false;
  bool co2dosing = false;
};

ActuatorStates currentActuators;

// Simulation data
struct SimulationData {
  bool active = false;
  String type = "none";
  unsigned long startTime = 0;
  int duration = 10;
  float simTemp = 0;
  float simHumidity = 0;
  float simSoilMoisture = 0;
  float simGasLevel = 0;
  float simLightLevel = 0;
};

SimulationData simulationState;

// Timing variables
unsigned long lastSensorRead = 0;
unsigned long lastFirebaseUpdate = 0;
unsigned long lastSimulationCheck = 0;

// Function prototypes
void setupWiFi();
void setupFirebase();
void setupSensors();
void setupActuators();
void readSensors();
void updateFirebaseSensors();
void checkModeChange();
void runNormalMode();
void runSimulationMode();
void controlActuators(ActuatorStates states);
void updateFirebaseActuators(ActuatorStates states, bool isSimulation);
ActuatorStates calculateNormalModeActuators();
ActuatorStates calculateSimulationActuators();
float readSoilMoisture();
float readGasLevel();
float readLightLevel();

// Helper: safe path builder using concat()
String buildPath(const char* p1, const char* p2 = nullptr, const char* p3 = nullptr, const char* p4 = nullptr) {
  String path;
  path.reserve(128);
  if (p1) path.concat(p1);
  if (p2) path.concat(p2);
  if (p3) path.concat(p3);
  if (p4) path.concat(p4);
  return path;
}

void setup() {
  Serial.begin(9600);
  Serial.println("Kultivate Greenhouse Control Starting...");
  
  pinMode(STATUS_LED, OUTPUT);
  
  setupWiFi();
  setupFirebase();
  setupSensors();
  setupActuators();

  // Wait for sensors to stabilize
  delay(2000);
  readSensors();

  controlActuators(currentActuators);
  updateFirebaseActuators(currentActuators, false);
  
  Serial.println("System ready!");
  digitalWrite(STATUS_LED, HIGH);
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Read sensors periodically
  if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
    lastSensorRead = currentMillis;
    readSensors();
    updateFirebaseSensors();
  }
  
  // Check for mode changes
  if (currentMillis - lastSimulationCheck >= SIMULATION_CHECK_INTERVAL) {
    lastSimulationCheck = currentMillis;
    checkModeChange();
  }
  
  // Run appropriate mode logic
  if (currentMode == NORMAL_MODE) {
    runNormalMode();
  } else if (currentMode == SIMULATION_MODE) {
    runSimulationMode();
  }
}

void setupWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void setupFirebase() {
  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_ST; // ✅ Add this line

  Firebase.begin(&config, nullptr);
  Firebase.reconnectWiFi(true);
  signupOK = true;

  String path;
  path.concat("greenhouses/");
  path.concat(ZONE_ID);
  path.concat("/mode");
  
  if (Firebase.RTDB.setString(&fbdo, path.c_str(), "normal")) {
    Serial.println("Firebase write successful!");
  } else {
    Serial.printf("Firebase write failed: %s\n", fbdo.errorReason().c_str());
  }
}


void setupSensors() {
  dht.begin();
  pinMode(SOIL_MOISTURE_PIN, INPUT);
  pinMode(GAS_SENSOR_PIN, INPUT);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  
  Serial.println("Sensors initialized");
}

void setupActuators() {
  pinMode(FAN_PIN, OUTPUT);
  pinMode(FAN_PIN_2, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(MISTING_PIN, OUTPUT);
  pinMode(LIGHTING_PIN, OUTPUT);
  pinMode(CO2_DOSING_PIN, OUTPUT);
  
  // Start with all actuators off
  digitalWrite(FAN_PIN, HIGH ); // Assuming active LOW for fan
  digitalWrite(FAN_PIN_2, HIGH ); // Assuming active LOW for fan
  digitalWrite(PUMP_PIN, HIGH); // Assuming active LOW for pump
  digitalWrite(HEATER_PIN, HIGH);
  digitalWrite(MISTING_PIN, HIGH);
  digitalWrite(LIGHTING_PIN, HIGH);
  digitalWrite(CO2_DOSING_PIN, HIGH);
  
  Serial.println("Actuators initialized");
}

void readSensors() {
  currentSensors.temperature = dht.readTemperature();
  currentSensors.humidity = dht.readHumidity();
  currentSensors.soilMoisture = readSoilMoisture();
  currentSensors.gasLevel = readGasLevel();
  currentSensors.lightLevel = readLightLevel();
  
  // Validate readings
  if (isnan(currentSensors.temperature)) currentSensors.temperature = 0;
  if (isnan(currentSensors.humidity)) currentSensors.humidity = 0;
  
  Serial.printf("Sensors - Temp: %.1f°C, Humidity: %.1f%%, Soil: %.1f%%, Gas: %.1f ppm, Light: %.1f lux\n",
                currentSensors.temperature, currentSensors.humidity, 
                currentSensors.soilMoisture, currentSensors.gasLevel,
                currentSensors.lightLevel);
}

float readSoilMoisture() {
  int rawValue = analogRead(SOIL_MOISTURE_PIN);
  // Convert to percentage (calibrate these values for your sensor)
  float moisture = map(rawValue, 4095, 0, 0, 100);
  return constrain(moisture, 0, 100);
}

float readGasLevel() {
  int rawValue = analogRead(GAS_SENSOR_PIN);
  // Convert to CO2 ppm (calibrate for your sensor)
  float gasLevel = map(rawValue, 0, 4095, 300, 2000);
  return gasLevel;
}

float readLightLevel() {
  int rawValue = analogRead(LIGHT_SENSOR_PIN);
  // Convert to lux (calibrate for your sensor)
  float lightLevel = map(rawValue, 0, 4095, 0, 1000);
  return lightLevel;
}

void updateFirebaseSensors() {
  if (Firebase.ready() && signupOK) {
    String path = buildPath("greenhouses/", ZONE_ID, "/sensors");
    
    FirebaseJson json;
    json.set("temperature", currentSensors.temperature);
    json.set("humidity", currentSensors.humidity);
    json.set("soilMoisture", currentSensors.soilMoisture);
    json.set("gasLevel", currentSensors.gasLevel);
    json.set("lightLevel", currentSensors.lightLevel);
    json.set("lastUpdate", (long)millis());
    
    Firebase.RTDB.updateNode(&fbdo, path.c_str(), &json);
  }
}

void checkModeChange() {
  if (Firebase.ready() && signupOK) {
    String path = buildPath("greenhouses/", ZONE_ID, "/mode");
    
    if (Firebase.RTDB.getString(&fbdo, path.c_str())) {
      String mode = fbdo.stringData();
      
      if (mode == "simulation" && currentMode == NORMAL_MODE) {
        Serial.println("Switching to SIMULATION MODE");
        currentMode = SIMULATION_MODE;
        
        // Load simulation parameters
        path = buildPath("greenhouses/", ZONE_ID, "/simulation");
        if (Firebase.RTDB.getJSON(&fbdo, path.c_str())) {
          FirebaseJson &json = fbdo.jsonObject();
          FirebaseJsonData result;
          
          json.get(result, "type");
          simulationState.type = result.stringValue;
          
          json.get(result, "duration");
          simulationState.duration = result.intValue;
          
          json.get(result, "conditions/temperature");
          simulationState.simTemp = result.floatValue;
          
          json.get(result, "conditions/humidity");
          simulationState.simHumidity = result.floatValue;
          
          json.get(result, "conditions/soilMoisture");
          simulationState.simSoilMoisture = result.floatValue;
          
          json.get(result, "conditions/gasLevel");
          simulationState.simGasLevel = result.floatValue;
          
          json.get(result, "conditions/lightLevel");
          simulationState.simLightLevel = result.floatValue;
          
          simulationState.active = true;
          simulationState.startTime = millis();
          
          Serial.printf("Simulation loaded: %s for %d seconds\n", 
                       simulationState.type.c_str(), simulationState.duration);
        }
      } else if (mode == "normal" && currentMode == SIMULATION_MODE) {
        Serial.println("Switching to NORMAL MODE");
        currentMode = NORMAL_MODE;
        simulationState.active = false;
        lastModeChangeTime = millis();
      }
      
    }
  }
}

void runNormalMode() {
  if (millis() - lastModeChangeTime < MODE_CHANGE_COOLDOWN) {
    return; // Still in cooldown period after simulation, do nothing
  }

  // Calculate actuator states based on sensor readings
  ActuatorStates newStates = calculateNormalModeActuators();
  
  // Only update if states changed
  if (memcmp(&newStates, &currentActuators, sizeof(ActuatorStates)) != 0) {
    currentActuators = newStates;
    controlActuators(currentActuators);
    updateFirebaseActuators(currentActuators, false);
  }
}

ActuatorStates calculateNormalModeActuators() {
  ActuatorStates states;
  
  // Fan control (cooling/ventilation)
  if (currentSensors.temperature > normalParams.targetTempMax) {
    states.fan = true;
  } else if (currentSensors.temperature < normalParams.targetTempMin) {
    states.fan = false;
  } else {
    states.fan = currentActuators.fan; // Maintain current state
  }
  
  // Heater control
  if (currentSensors.temperature < normalParams.targetTempMin) {
    states.heater = true;
  } else if (currentSensors.temperature > normalParams.targetTempMin + 2) {
    states.heater = false;
  } else {
    states.heater = currentActuators.heater;
  }
  
  // Irrigation pump (for soil moisture)
  if (currentSensors.soilMoisture < normalParams.targetSoilMoistureMin) {
    states.pump = true;
  } else if (currentSensors.soilMoisture > normalParams.targetSoilMoistureMax) {
    states.pump = false;
  } else {
    states.pump = currentActuators.pump;
  }
  
  // Misting system (for humidity)
  if (currentSensors.humidity < normalParams.targetHumidityMin) {
    states.misting = true;
  } else if (currentSensors.humidity > normalParams.targetHumidityMax) {
    states.misting = false;
  } else {
    states.misting = currentActuators.misting;
  }
  
  // Lighting control
  if (currentSensors.lightLevel < normalParams.targetLightLevelMin) {
    states.lighting = true;
  } else {
    states.lighting = false;
  }
  
  // CO2 dosing
  if (currentSensors.gasLevel < normalParams.targetGasLevelMin) {
    states.co2dosing = true;
  } else {
    states.co2dosing = false;
  }
  
  return states;
}

void resetAllActuators() {

ActuatorStates offStates;

offStates.fan = false;

offStates.pump = false;

offStates.heater = false;

offStates.misting = false;

offStates.lighting = false;

offStates.co2dosing = false;

controlActuators(offStates);

updateFirebaseActuators(offStates, true); // Update simulation node

updateFirebaseActuators(offStates, false); // Update normal mode node too

currentActuators = offStates;

}

void runSimulationMode() {
  if (!simulationState.active) return;
  
  unsigned long elapsed = (millis() - simulationState.startTime) / 1000;
  
 // Check if simulation duration has elapsed

if (elapsed >= simulationState.duration) {

Serial.println("Simulation complete - returning to normal mode");


// Update Firebase to end simulation

if (Firebase.ready() && signupOK) {

String path = buildPath("greenhouses/", ZONE_ID, "/simulation/status");

Firebase.RTDB.setString(&fbdo, path.c_str(), "complete");


path = buildPath("greenhouses/", ZONE_ID, "/mode");

Firebase.RTDB.setString(&fbdo, path.c_str(), "normal");

}


simulationState.active = false;

currentMode = NORMAL_MODE;

return;

}

  if (elapsed >= simulationState.duration) {
  Serial.println("Simulation complete - returning to normal mode");

  // Turn off all actuators after simulation
  ActuatorStates offStates;
  offStates.fan = false;
  offStates.pump = false;
  offStates.heater = false;
  offStates.misting = false;
  offStates.lighting = false;
  offStates.co2dosing = false;
  controlActuators(offStates);
  updateFirebaseActuators(offStates, true); // Update simulation node
  updateFirebaseActuators(offStates, false); // Update normal mode node too
  currentActuators = offStates;

  // Update Firebase mode and simulation status
  if (Firebase.ready() && signupOK) {
    String path = buildPath("greenhouses/", ZONE_ID, "/simulation/status");
    Firebase.RTDB.setString(&fbdo, path.c_str(), "complete");

    path = buildPath("greenhouses/", ZONE_ID, "/mode");
    Firebase.RTDB.setString(&fbdo, path.c_str(), "normal");
  }

  simulationState.active = false;
  currentMode = NORMAL_MODE;
  Serial.println("Actuators reset after simulation.");
  return;
}

  
  // Calculate actuator responses to simulated conditions
  ActuatorStates simStates = calculateSimulationActuators();
  
  // Update actuators if states changed
  if (memcmp(&simStates, &currentActuators, sizeof(ActuatorStates)) != 0) {
    currentActuators = simStates;
    controlActuators(currentActuators);
    updateFirebaseActuators(currentActuators, true);
    
    Serial.printf("Simulation actuators updated (%lu/%d sec)\n", elapsed, simulationState.duration);
  }
}

ActuatorStates calculateSimulationActuators() {
  ActuatorStates states;
  
  // Respond to simulated conditions
  float tempDiff = simulationState.simTemp - normalParams.targetTempMax;
  
  // Heatwave scenario
  if (simulationState.type == "heatwave") {
    states.fan = true; // Cool down
    if (simulationState.simSoilMoisture < 30) {
      states.pump = true; // Prevent heat stress
    }
  }
  
  // Dry soil scenario
  else if (simulationState.type == "drysoil") {
    states.pump = true; // Irrigate immediately
    if (simulationState.simTemp > normalParams.targetTempMax) {
      states.fan = true; // Also cool if hot
    }
  }
  
  // Drought scenario (high temp + low soil moisture)
  else if (simulationState.type == "drought") {
    states.fan = true; // Cooling
    states.pump = true; // Irrigation
    states.misting = true; // Humidity to reduce transpiration
  }
  
  // Cold snap scenario
  else if (simulationState.type == "coldsnap") {
    states.heater = true; // Heat up
    states.fan = false; // Don't ventilate cold air
  }
  
  // High humidity scenario
  else if (simulationState.type == "highhumidity") {
    states.fan = true; // Ventilate to reduce humidity
    states.misting = false; // Stop misting
  }
  
  // Low light scenario
  else if (simulationState.type == "lowlight") {
    states.lighting = true; // Supplemental lighting
  }
  
  // CO2 deficiency scenario
  else if (simulationState.type == "co2deficiency") {
    states.co2dosing = true; // Add CO2
  }
  
  return states;
}

void controlActuators(ActuatorStates states) {
  digitalWrite(FAN_PIN, states.fan ? LOW : HIGH);
  digitalWrite(FAN_PIN_2, states.fan ? LOW : HIGH);
  digitalWrite(PUMP_PIN, states.pump ? LOW : HIGH);
  digitalWrite(HEATER_PIN, states.heater ? LOW : HIGH);
  digitalWrite(MISTING_PIN, states.misting ? LOW : HIGH);
  digitalWrite(LIGHTING_PIN, states.lighting ? LOW : HIGH);
  digitalWrite(CO2_DOSING_PIN, states.co2dosing ? LOW : HIGH);
  
  Serial.printf("Actuators - Fan:%d Pump:%d Heater:%d Mist:%d Light:%d CO2:%d\n",
                states.fan, states.pump, states.heater, 
                states.misting, states.lighting, states.co2dosing);
}

void updateFirebaseActuators(ActuatorStates states, bool isSimulation) {
  if (Firebase.ready() && signupOK) {
    String path;
    
    if (isSimulation) {
      path = buildPath("greenhouses/", ZONE_ID, "/simulation/actuatorStates");
    } else {
      path = buildPath("greenhouses/", ZONE_ID, "/normalMode/actuatorStates");
    }
    
    FirebaseJson json;
    json.set("fan", states.fan);
    json.set("pump", states.pump);
    json.set("heater", states.heater);
    json.set("misting", states.misting);
    json.set("lighting", states.lighting);
    json.set("co2dosing", states.co2dosing);
    
    Firebase.RTDB.updateNode(&fbdo, path.c_str(), &json);
  }
}
