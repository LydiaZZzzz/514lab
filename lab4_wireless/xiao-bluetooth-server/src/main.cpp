#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <stdlib.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long previousMillis = 0;
const long interval = 1000;

// TODO: add new global variables for your sensor readings and processed data
const int trigPin = 6;   
const int echoPin = 5;   

#define NUM_SAMPLES 5  
float distanceSamples[NUM_SAMPLES] = {0}; 
int sampleIndex = 0;

#define SERVICE_UUID        "705eac08-e46d-4df9-a1d9-7e87a7597e2d"
#define CHARACTERISTIC_UUID "9b85c59c-4170-4176-902c-8f2ee4a294e7"
// TODO: add DSP algorithm functions here
float getDistance() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH);
    return duration * 0.034 / 2; 
}

float movingAverageFilter(float newSample) {
    distanceSamples[sampleIndex] = newSample;
    sampleIndex = (sampleIndex + 1) % NUM_SAMPLES;

    float sum = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        sum += distanceSamples[i];
    }
    return sum / NUM_SAMPLES;
}

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    // TODO: add codes for handling your sensor setup (pinMode, etc.)
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    // TODO: name your device to avoid conflictions
    BLEDevice::init("ShangmingYunqing_Server");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setValue("Hello World");
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop() {
    // TODO: add codes for handling your sensor readings (analogRead, etc.)
    float rawDistance = getDistance();  
    float filteredDistance = movingAverageFilter(rawDistance); 

    // TODO: use your defined DSP algorithm to process the readings
    Serial.print("Raw Distance: ");
    Serial.print(rawDistance);
    Serial.print(" cm | Filtered Distance: ");
    Serial.print(filteredDistance);
    Serial.println(" cm");

    if (deviceConnected) {
        // Send new readings to database
        // TODO: change the following code to send your own readings and processed data
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
            if (filteredDistance < 100) {  
                char distanceStr[10];
                dtostrf(filteredDistance, 4, 2, distanceStr);
                pCharacteristic->setValue(distanceStr);
                pCharacteristic->notify();
                Serial.print("Transmitting over BLE: ");
                Serial.println(distanceStr);
            }
        }
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);
        pServer->startAdvertising();
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
    delay(1000);
}