// #include <Arduino.h>
// #include <WiFi.h>
// #include <Firebase_ESP_Client.h>
// #include <addons/TokenHelper.h>
// #include <addons/RTDBHelper.h>
// #include <DHT.h>

// // WiFi credentials
// #define WIFI_SSID     "Oracle"
// #define WIFI_PASSWORD "12345678"

// // Firebase credentials (no authentication required)
// #define API_KEY       "AIzaSyDN3eUAOAWhFBsrez4UrgtEKjfAAv57Y3g"
// #define DATABASE_URL  "https://evidence-3abac-default-rtdb.firebaseio.com/"

// // Pin definitions
// #define DHT_PIN 4
// #define DHT_PIN_2 15
// #define SOIL_MOISTURE_PIN 35
// #define GAS_SENSOR_PIN 34
// #define FAN_PIN 32 
// #define Fan_Pin_2 27
// #define PUMP_PIN 16 

// // #define HEATER_PIN 15 //27
// // #define MISTING_PIN 14
// // #define LIGHTING_PIN 12
// // #define CO2_DOSING_PIN 13

// // Sensor configuration
// #define DHT_TYPE DHT22
// DHT dht(DHT_PIN, DHT_TYPE);
// DHT dht_2(DHT_PIN_2, DHT_TYPE);

// // Firebase objects (v4.4.17 compatible)
// FirebaseData fbdo;
// FirebaseAuth auth;
// FirebaseConfig config;

// // Zone ID (configure per greenhouse)
// const char* zoneId = "zone-1";

// // Firebase paths
// String sensorsPath;
// String simulationPath;
// String actuatorsPath;

// // Data structures
// struct SensorData {
//   float temperature;
//   float humidity;
//   int soilMoisture;
//   int gasLevel;
//   unsigned long lastUpdate;
// } sensorData;

// struct SimulationData {
//   bool active;
//   String type;
//   unsigned long startTime;
//   int duration;
//   String status;
// } simulationData;

// struct ActuatorData {
//   bool fan;
//   bool pump;
//   bool heater;
//   bool misting;
//   bool lighting;
//   bool co2dosing;
// } actuatorData;

// // Timing variables
// unsigned long lastSensorRead = 0;
// unsigned long lastFirebaseUpdate = 0;
// unsigned long lastSimulationCheck = 0;
// const unsigned long SENSOR_READ_INTERVAL = 2000;    // 2 seconds
// const unsigned long FIREBASE_UPDATE_INTERVAL = 3000; // 3 seconds
// const unsigned long SIMULATION_CHECK_INTERVAL = 500; // 500ms

// // Firebase connection flag
// bool firebaseReady = false;

// // Function declarations
// void setupWiFi();
// void setupFirebase();
// void readSensors();
// void updateFirebaseSensors();
// void listenToSimulation();
// void listenToActuators();
// void controlActuators();
// void handleSimulation();
// void initializeFirebasePaths();
// void buildPaths();

// void setup() {
//   Serial.begin(9600);
//   delay(1000);
  
//   Serial.println("\n\n");
//   Serial.println("========================================");
//   Serial.println("  Kultivate Greenhouse Control System  ");
//   Serial.println("      Firebase v4.4.17 Edition         ");
//   Serial.println("========================================");
  
//   // Initialize pins
//   pinMode(FAN_PIN, OUTPUT);
//   pinMode(Fan_Pin_2, OUTPUT);
//   pinMode(PUMP_PIN, OUTPUT);
  
//   // Initialize all actuators to OFF
//   digitalWrite(FAN_PIN, HIGH);
//   digitalWrite(Fan_Pin_2, HIGH);
//   digitalWrite(PUMP_PIN, HIGH);
  
//   Serial.println("Actuator pins initialized");
  
//   // Initialize sensors
//   dht.begin();
//   dht_2.begin();
//   Serial.println("DHT22 sensor initialized");
  
//   // Build Firebase paths
//   buildPaths();
  
//   // Connect to WiFi
//   setupWiFi();
  
//   // Setup Firebase
//   setupFirebase();
  
//   // Initialize Firebase paths
//   initializeFirebasePaths();
  
//   // Initialize simulation data
//   simulationData.active = false;
//   simulationData.type = "none";
//   simulationData.startTime = 0;
//   simulationData.duration = 10000;
//   simulationData.status = "idle";
  
//   Serial.println("\nSystem initialized successfully");
//   Serial.println("Ready to receive commands\n");
// }

// void loop() {
//   unsigned long currentMillis = millis();
  
//   // Check if Firebase is ready
//   if (!firebaseReady) {
//     delay(1000);
//     return;
//   }
  
//   // Read sensors periodically
//   if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
//     readSensors();
//     lastSensorRead = currentMillis;
//   }
  
//   // Update Firebase periodically
//   if (currentMillis - lastFirebaseUpdate >= FIREBASE_UPDATE_INTERVAL) {
//     updateFirebaseSensors();
//     lastFirebaseUpdate = currentMillis;
//   }
  
//   // Check simulation status periodically
//   if (currentMillis - lastSimulationCheck >= SIMULATION_CHECK_INTERVAL) {
//     listenToSimulation();
//     listenToActuators();
//     lastSimulationCheck = currentMillis;
//   }
  
//   // Handle simulation logic
//   handleSimulation();
  
//   // Control actuators
//   controlActuators();
  
//   delay(100);
// }

// void buildPaths() {
//   // Build paths without using + operator
//   sensorsPath = "/greenhouses/";
//   sensorsPath.concat(zoneId);
//   sensorsPath.concat("/sensors");
  
//   simulationPath = "/greenhouses/";
//   simulationPath.concat(zoneId);
//   simulationPath.concat("/simulation");
  
//   actuatorsPath = "/greenhouses/";
//   actuatorsPath.concat(zoneId);
//   actuatorsPath.concat("/actuators");
  
//   Serial.println("Firebase paths configured:");
//   Serial.print("  Sensors: ");
//   Serial.println(sensorsPath);
//   Serial.print("  Simulation: ");
//   Serial.println(simulationPath);
//   Serial.print("  Actuators: ");
//   Serial.println(actuatorsPath);
// }

// void setupWiFi() {
//   Serial.print("Connecting to WiFi");
//   WiFi.mode(WIFI_STA);
//   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
//   int attempts = 0;
//   while (WiFi.status() != WL_CONNECTED && attempts < 30) {
//     delay(500);
//     Serial.print(".");
//     attempts++;
//   }
  
//   if (WiFi.status() == WL_CONNECTED) {
//     Serial.println("\nWiFi connected");
//     Serial.print("IP address: ");
//     Serial.println(WiFi.localIP());
//   } else {
//     Serial.println("\nWiFi connection failed!");
//     Serial.println("Restarting in 5 seconds...");
//     delay(5000);
//     ESP.restart();
//   }
// }

// void setupFirebase() {
//   Serial.println("\nInitializing Firebase...");
  
//   // Assign the RTDB URL (no authentication needed)
//   config.database_url = DATABASE_URL;
  
//   // Disable authentication tokens
//   config.signer.tokens.legacy_token = "jas65T0JYHiger39Z9jcWpIVELnmPF4MUnWf7xTX";
  
//   // Initialize Firebase without authentication
//   Firebase.begin(&config, &auth);
  
//   // Enable auto reconnection
//   Firebase.reconnectWiFi(true);
  
//   // Set database read timeout
//   fbdo.setResponseSize(4096);
  
//   // Wait for Firebase to be ready
//   Serial.print("Waiting for Firebase connection");
//   int attempts = 0;
//   while (!Firebase.ready() && attempts < 30) {
//     Serial.print(".");
//     delay(500);
//     attempts++;
//   }
  
//   if (Firebase.ready()) {
//     firebaseReady = true;
//     Serial.println("\nFirebase connected successfully");
//     Serial.println("No authentication required");
//   } else {
//     Serial.println("\nFirebase connection failed!");
//     Serial.println("Check your DATABASE_URL");
//   }
// }

// void initializeFirebasePaths() {
//   Serial.println("\nInitializing database structure...");
  
//   // Initialize sensor structure
//   FirebaseJson sensorJson;
//   sensorJson.set("temperature", 0);
//   sensorJson.set("humidity", 0);
//   sensorJson.set("soilMoisture", 0);
//   sensorJson.set("gasLevel", 0);
//   sensorJson.set("lastUpdate", (int)millis());
  
//   if (Firebase.RTDB.setJSON(&fbdo, sensorsPath.c_str(), &sensorJson)) {
//     Serial.println("Sensors structure initialized");
//   }
  
//   // Initialize simulation structure
//   FirebaseJson simJson;
//   simJson.set("active", false);
//   simJson.set("type", "none");
//   simJson.set("startTime", 0);
//   simJson.set("duration", 10000);
//   simJson.set("status", "idle");
  
//   if (Firebase.RTDB.setJSON(&fbdo, simulationPath.c_str(), &simJson)) {
//     Serial.println("Simulation structure initialized");
//   }
  
//   // Initialize actuators structure
//   FirebaseJson actuatorJson;
//   actuatorJson.set("fan", false);
//   actuatorJson.set("pump", false);
//   actuatorJson.set("heater", false);
//   actuatorJson.set("misting", false);
//   actuatorJson.set("lighting", false);
//   actuatorJson.set("co2dosing", false);
  
//   if (Firebase.RTDB.setJSON(&fbdo, actuatorsPath.c_str(), &actuatorJson)) {
//     Serial.println("Actuators structure initialized");
//   }
  
//   Serial.println("Database structure ready\n");
// }

// void readSensors() {
//   // Read temperature and humidity from DHT22
//   sensorData.temperature = (dht.readTemperature() + dht_2.readTemperature()) / 2;
//   sensorData.humidity = (dht.readHumidity() + dht_2.readHumidity()) / 2;
  
//   // Check for reading errors
//   if (isnan(sensorData.temperature)) {
//     sensorData.temperature = 0;
//   }
//   if (isnan(sensorData.humidity)) {
//     sensorData.humidity = 0;
//   }
  
//   // Read soil moisture (0-4095 analog value, convert to percentage)
//   int soilRaw = analogRead(SOIL_MOISTURE_PIN);
//   sensorData.soilMoisture = map(soilRaw, 4095, 0, 0, 100);
//   sensorData.soilMoisture = constrain(sensorData.soilMoisture, 0, 100);
  
//   // Read gas sensor (approximate CO2 ppm)
//   int gasRaw = analogRead(GAS_SENSOR_PIN);
//   sensorData.gasLevel = map(gasRaw, 0, 4095, 300, 1000);
//   sensorData.gasLevel = constrain(sensorData.gasLevel, 300, 1000);
  
//   sensorData.lastUpdate = millis();
  
//   // Print sensor readings
//   Serial.println("--- Sensor Readings ---");
//   Serial.print("Temperature:   ");
//   Serial.print(sensorData.temperature);
//   Serial.println(" C");
//   Serial.print("Humidity:      ");
//   Serial.print(sensorData.humidity);
//   Serial.println(" %");
//   Serial.print("Soil Moisture: ");
//   Serial.print(sensorData.soilMoisture);
//   Serial.println(" %");
//   Serial.print("Gas Level:     ");
//   Serial.print(sensorData.gasLevel);
//   Serial.println(" ppm");
//   Serial.println();
// }

// void updateFirebaseSensors() {
//   if (!firebaseReady) return;
  
//   FirebaseJson json;
//   json.set("temperature", sensorData.temperature);
//   json.set("humidity", sensorData.humidity);
//   json.set("soilMoisture", sensorData.soilMoisture);
//   json.set("gasLevel", sensorData.gasLevel);
//   json.set("lastUpdate", (int)sensorData.lastUpdate);
  
//   if (Firebase.RTDB.updateNode(&fbdo, sensorsPath.c_str(), &json)) {
//     Serial.println("Sensors updated to Firebase");
//   } else {
//     Serial.println("Failed to update sensors");
//     Serial.print("Error: ");
//     Serial.println(fbdo.errorReason());
//   }
// }

// void listenToSimulation() {
//   if (!firebaseReady) return;
  
//   if (Firebase.RTDB.getJSON(&fbdo, simulationPath.c_str())) {
//     FirebaseJson &json = fbdo.jsonObject();
//     FirebaseJsonData jsonData;
    
//     // Parse simulation data
//     if (json.get(jsonData, "active")) {
//       simulationData.active = jsonData.boolValue;
//     }
    
//     if (json.get(jsonData, "type")) {
//       simulationData.type = jsonData.stringValue;
//     }
    
//     if (json.get(jsonData, "startTime")) {
//       simulationData.startTime = jsonData.intValue;
//     }
    
//     if (json.get(jsonData, "duration")) {
//       simulationData.duration = jsonData.intValue;
//     }
    
//     if (json.get(jsonData, "status")) {
//       simulationData.status = jsonData.stringValue;
//     }
    
//     if (simulationData.active && simulationData.status == "running") {
//       Serial.print("Simulation active: ");
//       Serial.println(simulationData.type);
//     }
//   }
// }

// void listenToActuators() {
//   if (!firebaseReady) return;
  
//   if (Firebase.RTDB.getJSON(&fbdo, actuatorsPath.c_str())) {
//     FirebaseJson &json = fbdo.jsonObject();
//     FirebaseJsonData jsonData;
    
//     // Parse actuator commands
//     if (json.get(jsonData, "fan")) {
//       actuatorData.fan = jsonData.boolValue;
//     }
    
//     if (json.get(jsonData, "pump")) {
//       actuatorData.pump = jsonData.boolValue;
//     }
    
//     if (json.get(jsonData, "heater")) {
//       actuatorData.heater = jsonData.boolValue;
//     }
    
//     if (json.get(jsonData, "misting")) {
//       actuatorData.misting = jsonData.boolValue;
//     }
    
//     if (json.get(jsonData, "lighting")) {
//       actuatorData.lighting = jsonData.boolValue;
//     }
    
//     if (json.get(jsonData, "co2dosing")) {
//       actuatorData.co2dosing = jsonData.boolValue;
//     }
//   }
// }

// void controlActuators() {
//   // Control fan
//   digitalWrite(FAN_PIN, actuatorData.fan ? LOW : HIGH);

//   digitalWrite(Fan_Pin_2, actuatorData.fan ? LOW : HIGH);
  
//   // Control pump
//   digitalWrite(PUMP_PIN, actuatorData.pump ? LOW : HIGH);
  
//   // // Control heater
//   // digitalWrite(HEATER_PIN, actuatorData.heater ? HIGH : LOW);
  
//   // // Control misting
//   // digitalWrite(MISTING_PIN, actuatorData.misting ? HIGH : LOW);
  
//   // // Control lighting
//   // digitalWrite(LIGHTING_PIN, actuatorData.lighting ? HIGH : LOW);
  
//   // // Control CO2 dosing
//   // digitalWrite(CO2_DOSING_PIN, actuatorData.co2dosing ? HIGH : LOW);
  
//   // Print actuator status if any are active
//   if (actuatorData.fan || actuatorData.pump || actuatorData.heater || 
//       actuatorData.misting || actuatorData.lighting || actuatorData.co2dosing) {
    
//     Serial.println("--- Active Actuators ---");
//     if (actuatorData.fan) Serial.println("Fan:        ON");
//     if (actuatorData.pump) Serial.println("Pump:       ON");
//     if (actuatorData.heater) Serial.println("Heater:     ON");
//     if (actuatorData.misting) Serial.println("Misting:    ON");
//     if (actuatorData.lighting) Serial.println("Lighting:   ON");
//     if (actuatorData.co2dosing) Serial.println("CO2 Dosing: ON");
//     Serial.println();
//   }
// }

// void handleSimulation() {
//   if (!simulationData.active || simulationData.status != "running") {
//     return;
//   }
  
//   // Check if simulation duration has elapsed
//   unsigned long elapsed = millis() - simulationData.startTime;
  
//   if (elapsed >= simulationData.duration) {
//     // Simulation complete
//     Serial.println("Simulation completed");
    
//     FirebaseJson json;
//     json.set("active", false);
//     json.set("status", "complete");
    
//     Firebase.RTDB.updateNode(&fbdo, simulationPath.c_str(), &json);
    
//     // Turn off all actuators
//     actuatorData.fan = false;
//     actuatorData.pump = false;
//     actuatorData.heater = false;
//     actuatorData.misting = false;
//     actuatorData.lighting = false;
//     actuatorData.co2dosing = false;
    
//     FirebaseJson actuatorJson;
//     actuatorJson.set("fan", false);
//     actuatorJson.set("pump", false);
//     actuatorJson.set("heater", false);
//     actuatorJson.set("misting", false);
//     actuatorJson.set("lighting", false);
//     actuatorJson.set("co2dosing", false);
    
//     Firebase.RTDB.updateNode(&fbdo, actuatorsPath.c_str(), &actuatorJson);
    
//     simulationData.active = false;
//     simulationData.status = "complete";
    
//     Serial.println("System returning to normal operation\n");
//   } else {
//     // Print remaining time every 2 seconds
//     int remaining = (simulationData.duration - elapsed) / 1000;
//     static int lastRemaining = -1;
    
//     if (remaining != lastRemaining && remaining % 2 == 0) {
//       Serial.print("Simulation time remaining: ");
//       Serial.print(remaining);
//       Serial.println(" seconds");
//       lastRemaining = remaining;
//     }
//   }
// }