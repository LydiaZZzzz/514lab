#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

static BLEUUID serviceUUID("648e2beb-61a4-415d-8bee-4897853ba8c9");
static BLEUUID charUUID("9b78bc5d-7776-4368-8254-45b60db5ea4c");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

static std::vector<std::string> receivedDataBuffer;
static int receivedDataCount = 0;
static int dataSendCount = 0;

void aggregateReceivedData() {
    Serial.println("Aggregating Data...");
    
    std::string aggregatedData;
    for (const auto& data : receivedDataBuffer) {
        aggregatedData += data + " | ";
    }

    Serial.print("Aggregated Data: ");
    Serial.println(aggregatedData.c_str());

    receivedDataBuffer.clear();
    receivedDataCount = 0;
}

static float maxDistance = -9999.0;
static float minDistance = 9999.0;

static void notifyCallback(
    BLERemoteCharacteristic* pBLERemoteCharacteristic,
    uint8_t* pData,
    size_t length,
    bool isNotify) {

    std::string receivedStr(reinterpret_cast<char*>(pData), length);
    float distance = atof(receivedStr.c_str());

    if (distance > maxDistance) {
        maxDistance = distance;
    }
    if (distance < minDistance) {
        minDistance = distance;
    }

    dataSendCount++;

    Serial.println("--------------------------------------");
    Serial.printf("Received Distance: %.2f cm\n", distance);
    Serial.printf("Max Distance: %.2f cm\n", maxDistance);
    Serial.printf("Min Distance: %.2f cm\n", minDistance);
    Serial.printf("Total Data Received: %d\n", dataSendCount);
    Serial.println("--------------------------------------\n");

    receivedDataBuffer.push_back(receivedStr);
    receivedDataCount++;

    if (receivedDataCount >= 5) {
        aggregateReceivedData();
    }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {}

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("Disconnected from server.");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());

    BLEClient* pClient = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());
    pClient->connect(myDevice);
    Serial.println(" - Connected to server");
    pClient->setMTU(517); 

    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found service");

    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found characteristic");

    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("Characteristic initial value: ");
      Serial.println(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Client...");

  BLEDevice::deinit();
  delay(100);
  BLEDevice::init("");

  delay(5000);

  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->clearResults();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop() {
  if (doConnect) {
    BLEDevice::deinit();
    delay(100);
    BLEDevice::init("");

    if (connectToServer()) {
      Serial.println("Connected to the BLE Server.");
    } else {
      Serial.println("Failed to connect to the server, retrying...");
    }
    doConnect = false;
  }
 
  if (connected) {
    String newValue = "Time since boot: " + String(millis()/1000);
    Serial.println("Updating characteristic value: " + newValue);
    pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
  } else if (doScan) {
    BLEDevice::getScan()->start(0);
  }

  delay(1000);
}