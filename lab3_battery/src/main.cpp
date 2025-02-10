#include <WiFi.h>
#include <FirebaseClient.h>
#include <WiFiClientSecure.h>
#include "esp_wifi.h"
#include "esp_sleep.h"

#define TRIG_PIN 5
#define ECHO_PIN 18

#define WIFI_SSID "UW MPSK"
#define WIFI_PASSWORD "9L}Ch#!^6h"

#define DATABASE_SECRET "AIzaSyCyi0gTyJL9JBXA5c5UN5_NmdAl1mhpv9k"
#define DATABASE_URL "https://batterymanagementlab-default-rtdb.firebaseio.com/"

WiFiClientSecure ssl;
DefaultNetwork network;
AsyncClientClass client(ssl, getNetwork(network));
FirebaseApp app;
RealtimeDatabase Database;
AsyncResult result;
LegacyToken dbSecret(DATABASE_SECRET);

#define MOVEMENT_THRESHOLD 50.0  
#define MEASURE_INTERVAL 3000    
#define NO_MOVEMENT_SLEEP_INTERVAL 30000  
#define DEEP_SLEEP_DURATION 30000  
#define REQUIRED_DETECTIONS 3       
#define WAKEUP_CHECKS 3  

unsigned long lastMeasurementTime = 0;
unsigned long lastMovementTime = 0;
int movementCount = 0;

void connectToWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 5000) {
        delay(500);
    }
}

void initFirebase() {
    ssl.setInsecure();
    initializeApp(client, app, getAuth(dbSecret));
    app.getApp<RealtimeDatabase>(Database);
    Database.url(DATABASE_URL);
    client.setAsyncResult(result);
}

float measureDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH, 30000);
    float distance = (duration * 0.0343f) / 2.0f;
    return distance;
}

void disconnectWiFi() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    esp_wifi_stop();
}

void sendDataToFirebase(float distance) {
    connectToWiFi();
    initFirebase();
    Database.set<float>(client, "/sensor/distance", distance);
    disconnectWiFi();
}

void enterDeepSleep() {
    disconnectWiFi();
    esp_sleep_enable_timer_wakeup(DEEP_SLEEP_DURATION * 1000ULL);
    esp_deep_sleep_start();
}

bool checkAfterWakeup() {
    int inactiveCount = 0;
    for (int i = 0; i < WAKEUP_CHECKS; i++) {
        delay(MEASURE_INTERVAL);
        float distance = measureDistance();
        if (distance == 0 || distance > MOVEMENT_THRESHOLD) {
            inactiveCount++;
        }
    }
    return inactiveCount == WAKEUP_CHECKS;
}

void setup() {
    Serial.begin(115200);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    WiFi.mode(WIFI_OFF);
    esp_wifi_stop();

    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
        if (checkAfterWakeup()) {
            enterDeepSleep();
        }
    }
}

void loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastMeasurementTime >= MEASURE_INTERVAL) {
        lastMeasurementTime = currentMillis;
        float distance = measureDistance();

        if (distance > 0 && distance < MOVEMENT_THRESHOLD) {
            movementCount++;
            lastMovementTime = currentMillis;
        } else {
            movementCount = 0;
        }

        if (movementCount >= REQUIRED_DETECTIONS) {
            sendDataToFirebase(distance);
            movementCount = 0;
        }
    }

    if (millis() - lastMovementTime > NO_MOVEMENT_SLEEP_INTERVAL) {
        enterDeepSleep();
    }
}