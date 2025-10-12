#include "firebase_manager.h"
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define API_KEY       "AIzaSyDN3eUAOAWhFBsrez4UrgtEKjfAAv57Y3g"
#define DATABASE_URL  "https://evidence-3abac-default-rtdb.firebaseio.com/"
#define USER_EMAIL    "amosmarvellous48@gmail.com"
#define USER_PASS     "qwerty7890"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

void initializeFirebase() {
  Serial.println("Initializing Firebase...");
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASS;
  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  unsigned long start = millis();
  while (!Firebase.ready() && millis() - start < 30000) {
    Serial.print(".");
    delay(500);
  }

  if (Firebase.ready()) {
    signupOK = true;
    Serial.println("\nFirebase connected!");
  } else {
    Serial.println("\nFirebase init failed!");
  }
}

bool sendRealtimeData(float temp, float hum, int soil, int gas) {
  if (!Firebase.ready() || !signupOK) return false;

  bool ok1 = Firebase.RTDB.setFloat(&fbdo, "/greenhouse/temperature", temp);
  bool ok2 = Firebase.RTDB.setFloat(&fbdo, "/greenhouse/humidity", hum);
  bool ok3 = Firebase.RTDB.setInt(&fbdo, "/greenhouse/soilMoisture", soil);
  bool ok4 = Firebase.RTDB.setInt(&fbdo, "/greenhouse/gasLevel", gas);
  bool ok5 = Firebase.RTDB.setTimestamp(&fbdo, "/greenhouse/lastUpdate");

  return ok1 && ok2 && ok3 && ok4 && ok5;
}
