#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

#include <Firebase_ESP_Client.h>

extern FirebaseData fbdo;
extern FirebaseAuth auth;
extern FirebaseConfig config;

void initializeFirebase();
bool sendRealtimeData(float temp, float hum, int soil, int gas);

#endif
