#include "wifiManager/wifi_manager.h"
#include "firebaseManager/firebase_manager.h"
#include "sensors/sensors.h"
#include "logger/logger.h"
#include "actuators/actuators.h"

unsigned long lastRead = 0;
const unsigned long INTERVAL = 5000; // 10 seconds

void setup() {
  Serial.begin(9600);
  connectToWiFi();
  setupTime();
  initializeFirebase();
  initSensors();
  Serial.println("System Ready!");
}

void loop() {
  if (millis() - lastRead > INTERVAL) {
    DHTReadings dht = readDHTSensors();
    float avgTemp = (dht.temperature1 + dht.temperature2) / 2.0;
    float avgHum  = (dht.humidity1 + dht.humidity2) / 2.0;
    int soil = readSoilSensor();
    int gas  = readGasSensor();

    if (sendRealtimeData(avgTemp, avgHum, soil, gas))
      logSensorData(avgTemp, avgHum, soil, gas);

    lastRead = millis();
  }
}


// // esp32_sim_integration.ino
// #include <WiFi.h>
// #include <Firebase_ESP_Client.h>
// #include <ArduinoJson.h>

// // Replace with your WiFi credentials
// const char* ssid = "YOUR_SSID";
// const char* password = "YOUR_PASS";

// // Firebase project credentials
// #define API_KEY "YOUR_FIREBASE_API_KEY"
// #define DATABASE_URL "https://<your-project>.firebaseio.com/"  // include trailing slash

// // Define your device/housing id
// const char* houseId = "house-001";

// // FB objects
// FirebaseData fbdo;
// FirebaseAuth auth;
// FirebaseConfig config;

// // actuator pins (example)
// const int FAN_PIN = 16;
// const int PUMP_PIN = 17;

// // simulation listener path
// String simPath;

// // sensor read interval
// unsigned long lastSensorTs = 0;
// const unsigned long sensorInterval = 2000; // every 2s

// void setup() {
//   Serial.begin(115200);
//   pinMode(FAN_PIN, OUTPUT);
//   pinMode(PUMP_PIN, OUTPUT);
//   digitalWrite(FAN_PIN, LOW);
//   digitalWrite(PUMP_PIN, LOW);

//   WiFi.begin(ssid, password);
//   Serial.print("Connecting WiFi");
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(300);
//     Serial.print(".");
//   }
//   Serial.println("\nConnected to WiFi");

//   config.api_key = API_KEY;
//   config.database_url = DATABASE_URL;

//   // No user sign-in required for RTDB reads if DB rules allow; otherwise set user token.
//   Firebase.begin(&config, &auth);

//   // Simulation path
//   simPath = "/houses/" + String(houseId) + "/simulations/current";
//   // Attach streaming / listener
//   if (!Firebase.RTDB.beginStream(&fbdo, simPath.c_str())) {
//     Serial.printf("Could not begin stream, reason: %s\n", fbdo.errorReason().c_str());
//   } else {
//     Firebase.RTDB.setStreamCallback(
//       &fbdo,
//       streamCallback,
//       streamTimeoutCallback
//     );
//   }
// }

// void loop() {
//   unsigned long now = millis();
//   if (now - lastSensorTs > sensorInterval) {
//     lastSensorTs = now;
//     sendSensorReadings();
//   }

//   // Keep Firebase stream alive (the library handles callbacks in background)
//   delay(10);
// }

// void sendSensorReadings() {
//   // Mock sensor reads — replace with real sensor code
//   float temp = 28.0 + random(-50, 50) / 10.0; // 23-33
//   float humidity = 60.0 + random(-200, 200) / 10.0;
//   float soil = 0.25 + random(-50, 50) / 100.0;

//   String path = "/houses/" + String(houseId) + "/sensors/latest";
//   FirebaseJson json;
//   json.set("temp", temp);
//   json.set("humidity", humidity);
//   json.set("soil", soil);
//   json.set("timestamp", (long) (millis())); // prefer epoch ms from an RTC or NTP

//   if (Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json)) {
//     // ok
//   } else {
//     Serial.println("Failed to write sensors: " + fbdo.errorReason());
//   }

//   // Also append to history if desired
//   // Firebase.RTDB.pushJSON(&fbdo, "/houses/house-001/sensors/history", &json);
// }

// // Firebase stream callbacks
// void streamCallback(FirebaseStream data) {
//   Serial.println("Stream event:");
//   Serial.println("Path: " + data.dataPath());
//   Serial.println("Type: " + data.eventType());
//   String payload = data.to<const String &>();
//   Serial.println(payload);

//   // parse payload as JSON
//   FirebaseJson json(payload);
//   FirebaseJsonData jsonData;

//   // if there is no simulation (cleared), turn off actuators?
//   if (payload == "null") {
//     Serial.println("Simulation cleared. Returning to normal controls.");
//     // Optionally stop actuators or revert to normal autonomous behavior
//     digitalWrite(FAN_PIN, LOW);
//     digitalWrite(PUMP_PIN, LOW);
//     return;
//   }

//   // parse simulation object
//   // payload looks like {"id":"...","type":"heatwave", "params": {...}, "startAt":..., "duration":10, "active":true}
//   String simType;
//   json.get(jsonData, "type");
//   if (jsonData.success) simType = jsonData.stringValue;

//   // simplistic reaction logic based on sim type + sensor snapshot
//   // read latest sensors from DB synchronously (or keep a local copy updated from sensors)
//   // For demo, we'll use the last local sensor readings (not ideal — better read /sensors/latest path)
//   // Here, we'll read sensors from DB:
//   String sensorsPath = "/houses/" + String(houseId) + "/sensors/latest";
//   if (Firebase.RTDB.getJSON(&fbdo, sensorsPath.c_str())) {
//     FirebaseJson sensorJson = fbdo.to<FirebaseJson>();
//     FirebaseJsonData jd;
//     sensorJson.get(jd, "temp");
//     float temp = jd.success ? jd.doubleValue : 25.0;
//     sensorJson.get(jd, "soil");
//     float soil = jd.success ? jd.doubleValue : 0.4;
//     Serial.printf("Sensor snapshot: temp=%.2f soil=%.2f\n", temp, soil);

//     // Example actuations:
//     if (simType == "heatwave") {
//       if (temp > 30.0) digitalWrite(FAN_PIN, HIGH);
//       else digitalWrite(FAN_PIN, LOW);

//       // If heat too high and soil low: start irrigation
//       if (temp > 30.0 && soil < 0.2) {
//         digitalWrite(PUMP_PIN, HIGH);
//       } else {
//         digitalWrite(PUMP_PIN, LOW);
//       }
//     } else if (simType == "drysoil") {
//       // directly start pump if soil <= target (params may include target)
//       sensorJson.get(jd, "soil");
//       float s = jd.success ? jd.doubleValue : soil;
//       if (s < 0.15) digitalWrite(PUMP_PIN, HIGH);
//       else digitalWrite(PUMP_PIN, LOW);
//     } else {
//       // default: revert to safe state
//       digitalWrite(FAN_PIN, LOW);
//       digitalWrite(PUMP_PIN, LOW);
//     }
//   } else {
//     Serial.println("Failed to fetch sensors for decision: " + fbdo.errorReason());
//   }
// }

// void streamTimeoutCallback(bool timeout) {
//   if (timeout) {
//     Serial.println("Stream timeout, resuming...");
//   }
// }
