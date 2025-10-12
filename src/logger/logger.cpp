#include "logger.h"
#include "firebaseManager/firebase_manager.h"

void setupTime() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  delay(2000);
}

String getDateString() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "unknown";
  char buffer[11];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d", &timeinfo);
  return String(buffer);
}

void logSensorData(float temp, float hum, int soil, int gas) {
  if (!Firebase.ready()) return;

  time_t now;
  time(&now);
  unsigned long ts = (unsigned long)now;
  String date = getDateString();
  char buffer[100];
  sprintf(buffer, "/greenhouse/logs/%s/%lu", date.c_str(), ts);
  String path = buffer;

  FirebaseJson json;
  json.set("temperature", temp);
  json.set("humidity", hum);
  json.set("soilMoisture", soil);
  json.set("gasLevel", gas);
  json.set("timestamp", ts);

  if (Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json)) {
    Serial.println("✅ Data logged successfully");
  } else {
    Serial.printf("❌ Log failed: %s\n", fbdo.errorReason().c_str());
  }
}
