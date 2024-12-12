#include <DHT11.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <ArduinoJson.h>
#include <Update.h>
#include "FS.h"
#include "FFat.h"
#include "SPIFFS.h"
#include <BLE2902.h>

#define BUILTINLED 2
#define FORMAT_SPIFFS_IF_FAILED true
#define FORMAT_FFAT_IF_FAILED true

#define USE_SPIFFS  // Comment to use FFat

#ifdef USE_SPIFFS
#define FLASH SPIFFS
#define FASTMODE false    // SPIFFS write is slow
#else
#define FLASH FFat
#define FASTMODE true     // FFat is faster
#endif

#define NORMAL_MODE   0   // Normal operation
#define UPDATE_MODE   1   // Receiving firmware
#define OTA_MODE      2   // Installing firmware

uint8_t updater[16384];
uint8_t updater2[16384];

// OTA Update Service UUIDs
#define OTA_SERVICE_UUID              "fb1e4001-54ae-4a28-9f74-dfccb248601d"
#define OTA_CHARACTERISTIC_UUID_RX    "fb1e4002-54ae-4a28-9f74-dfccb248601d"
#define OTA_CHARACTERISTIC_UUID_TX    "fb1e4003-54ae-4a28-9f74-dfccb248601d"

// Sensor Data Service UUIDs
#define SENSOR_SERVICE_UUID           "2d96526e-cc98-4ac0-a58f-55cd82d5c634"
#define SENSOR_CHARACTERISTIC_UUID    "00dc83ab-1db8-4f44-a78d-cf754866f003"

// BLE Server and Characteristics
BLEServer *pServer = NULL;
BLEService *pOtaService = NULL;
BLEService *pSensorService = NULL;

BLECharacteristic *pTxCharacteristic = NULL;        // OTA TX Characteristic
BLECharacteristic *pRxCharacteristic = NULL;        // OTA RX Characteristic
BLECharacteristic *pSensorCharacteristic = NULL;    // Sensor Data Characteristic

static bool deviceConnected = false, sendMode = false, sendSize = true;
static bool writeFile = false, request = false;
static int writeLen = 0, writeLen2 = 0;
static bool current = true;
static int parts = 0, next = 0, cur = 0, MTU = 0;
static int MODE = NORMAL_MODE;
unsigned long rParts, tParts;

//Initialize 
int humidity = 5;
int analog_reading = 5;
int digital_reading = 5;

// Pin configuration
DHT11 dht11(4);  // D4
const int AO_PIN = 32;  // GPIO 32
const int DO_PIN = 13;  // GPIO 13

StaticJsonDocument<60> jsonDoc;

void rebootEspWithReason(String reason) {
  Serial.println(reason);
  delay(1000);
  ESP.restart();
}

// Server callbacks
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Device connected");
    jsonDoc["Humidity"] = humidity;
    jsonDoc["Moisture"] = analog_reading;
    //jsonDoc["analog"] = analog_reading;
    //jsonDoc["digital"] = digital_reading;

    // Serialize JSON to a string
    char jsonBuffer[60];
    char trimmedData[60];
    size_t jsonSize = measureJson(jsonDoc);
    if (jsonSize > sizeof(jsonBuffer)) {
      Serial.println("Error: JSON size exceeds buffer size");
      delay(1000);
      return;
    }
    serializeJson(jsonDoc, jsonBuffer);

    // Print the JSON string for debugging
    Serial.println(jsonBuffer);

    Serial.println("Sending data over BLE");
    // Send the JSON string over BLE
    if (pSensorCharacteristic) {
      strncpy(trimmedData, jsonBuffer + 1, strlen(jsonBuffer) - 2);
      trimmedData[strlen(jsonBuffer) - 2] = '\0';
      Serial.println(trimmedData);
      pSensorCharacteristic->setValue(trimmedData);
      pSensorCharacteristic->notify();
      Serial.println("Data sent successfully");
    } else {
      Serial.println("Error: pSensorCharacteristic is null!");
    }
    // pSensorCharacteristic->setValue();
    // pSensorCharacteristic->notify();

  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Device disconnected");
    BLEDevice::startAdvertising();
    Serial.println("Device advertising agian");
  }
};

// OTA characteristic callbacks
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      uint8_t* pData;
      String value = pCharacteristic->getValue();
      int len = value.length();
      pData = pCharacteristic->getData();

      if (pData != NULL) {
        // Handle OTA update data
        if (pData[0] == 0xFB) {
          int pos = pData[1];
          for (int x = 0; x < len - 2; x++) {
            if (current) {
              updater[(pos * MTU) + x] = pData[x + 2];
            } else {
              updater2[(pos * MTU) + x] = pData[x + 2];
            }
          }
        } else if (pData[0] == 0xFC) {
          if (current) {
            writeLen = (pData[1] << 8) + pData[2];
          } else {
            writeLen2 = (pData[1] << 8) + pData[2];
          }
          current = !current;
          cur = (pData[3] << 8) + pData[4];
          writeFile = true;
          if (cur < parts - 1) {
            request = !FASTMODE;
          }
        } else if (pData[0] == 0xFD) {
          sendMode = true;
          if (FLASH.exists("/update.bin")) {
            FLASH.remove("/update.bin");
          }
        } else if (pData[0] == 0xFE) {
          rParts = 0;
          tParts = ((uint32_t)pData[1] << 24) | ((uint32_t)pData[2] << 16) | ((uint32_t)pData[3] << 8) | pData[4];
          Serial.print("Available space: ");
          Serial.println(FLASH.totalBytes() - FLASH.usedBytes());
          Serial.print("File Size: ");
          Serial.println(tParts);
        } else if (pData[0] == 0xFF) {
          parts = (pData[1] << 8) + pData[2];
          MTU = (pData[3] << 8) + pData[4];
          MODE = UPDATE_MODE;
        } else if (pData[0] == 0xEF) {
          FLASH.format();
          sendSize = true;
        }
      }
    }
};


void writeBinary(fs::FS &fs, const char * path, uint8_t *dat, int len) {
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  file.write(dat, len);
  file.close();
  writeFile = false;
  rParts += len;
}

void sendOtaResult(String result) {
  pTxCharacteristic->setValue(result.c_str());
  pTxCharacteristic->notify();
  delay(200);
}

void performUpdate(Stream &updateSource, size_t updateSize) {
  char s1 = 0x0F;
  String result = String(s1);

  if (Update.begin(updateSize)) {
    size_t written = Update.writeStream(updateSource);
    if (written == updateSize) {
      Serial.println("Written : " + String(written) + " successfully");
    } else {
      Serial.println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
    }
    result += "Written : " + String(written) + "/" + String(updateSize) + " [" + String((written / updateSize) * 100) + "%] \n";
    if (Update.end()) {
      Serial.println("OTA done!");
      result += "OTA Done: ";
      if (Update.isFinished()) {
        Serial.println("Update successfully completed. Rebooting...");
        result += "Success!\n";
      } else {
        Serial.println("Update not finished? Something went wrong!");
        result += "Failed!\n";
      }
    } else {
      Serial.println("Error Occurred. Error #: " + String(Update.getError()));
      result += "Error #: " + String(Update.getError());
    }
  } else {
    Serial.println("Not enough space to begin OTA");
    result += "Not enough space for OTA";
  }

  if (deviceConnected) {
    sendOtaResult(result);
    delay(5000);
  }
}

void updateFromFS(fs::FS &fs) {
  File updateBin = fs.open("/update.bin");
  if (updateBin) {
    if (updateBin.isDirectory()) {
      Serial.println("Error, update.bin is not a file");
      updateBin.close();
      return;
    }

    size_t updateSize = updateBin.size();
    if (updateSize > 0) {
      Serial.println("Trying to start update");
      performUpdate(updateBin, updateSize);
    } else {
      Serial.println("Error, file is empty");
    }

    updateBin.close();
    Serial.println("Removing update file");
    fs.remove("/update.bin");

    rebootEspWithReason("Rebooting to complete OTA update");
  } else {
    Serial.println("Could not load update.bin from spiffs root");
  }
}

void initBLE() {
  Serial.println("Starting BLE work!");
  BLEDevice::init("ESP32 IOT OTA");

  // Get and print the MAC address
  String macAddress = BLEDevice::getAddress().toString().c_str();
  Serial.print("Device MAC Address: ");
  Serial.println(macAddress);

  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());


  // Create OTA Update Service
  pOtaService = pServer->createService(OTA_SERVICE_UUID);

  // Create OTA characteristics
  pTxCharacteristic = pOtaService->createCharacteristic(
                        OTA_CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
  pRxCharacteristic = pOtaService->createCharacteristic(
                        OTA_CHARACTERISTIC_UUID_RX,
                        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
                      );

  pRxCharacteristic->setCallbacks(new MyCallbacks());
  pTxCharacteristic->addDescriptor(new BLE2902());
  pTxCharacteristic->setNotifyProperty(true);

  pOtaService->start();

  // Create Sensor Data Service
  pSensorService = pServer->createService(SENSOR_SERVICE_UUID);

  // Create Sensor Data characteristic
  pSensorCharacteristic = pSensorService->createCharacteristic(
                            SENSOR_CHARACTERISTIC_UUID,
                            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
                          );

  pSensorCharacteristic->addDescriptor(new BLE2902());
  pSensorCharacteristic->setNotifyProperty(true);

  pSensorService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(OTA_SERVICE_UUID);
  pAdvertising->addServiceUUID(SENSOR_SERVICE_UUID);

  // Add custom service data (optional)
  std::string otaServiceData = "OTA Update Service";
  std::string sensorServiceData = "Sensor Data Service";
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("BLE Services and Characteristics defined!");
}

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(115200);
  pinMode(BUILTINLED, OUTPUT);

#ifdef USE_SPIFFS
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
#else
  if (!FFat.begin()) {
    Serial.println("FFat Mount Failed");
    if (FORMAT_FFAT_IF_FAILED) FFat.format();
    return;
  }
#endif
  initBLE();
  dht11.setDelay(2000);
}

void loop() {
  switch (MODE) {
    case NORMAL_MODE:
      if (deviceConnected) {
        digitalWrite(BUILTINLED, HIGH);
        if (sendMode) {
          uint8_t fMode[] = {0xAA, FASTMODE};
          pTxCharacteristic->setValue(fMode, 2);
          pTxCharacteristic->notify();
          delay(50);
          sendMode = false;
            
        } else {
          digitalWrite(BUILTINLED, LOW);
          }

        }

        if (sendSize) {
          unsigned long x = FLASH.totalBytes();
          unsigned long y = FLASH.usedBytes();
          uint8_t fSize[] = {0xEF, (uint8_t)(x >> 16), (uint8_t)(x >> 8), (uint8_t)x, (uint8_t)(y >> 16), (uint8_t)(y >> 8), (uint8_t)y};
          pTxCharacteristic->setValue(fSize, 7);
          pTxCharacteristic->notify();
          delay(50);
          sendSize = false;
        }
        
        // Read sensor data
        humidity = dht11.readHumidity();
        analog_reading = analogRead(AO_PIN);
        digital_reading = digitalRead(DO_PIN);

        if (humidity != DHT11::ERROR_CHECKSUM && humidity != DHT11::ERROR_TIMEOUT) {
          Serial.print("Humidity: ");
          Serial.print(humidity);
          Serial.println(" %");
        } else {
          Serial.println(DHT11::getErrorString(humidity));
        }

        Serial.print("Reading raw value: ");
        Serial.println(analog_reading);
        Serial.print("Reading digital output: ");
        Serial.println(digital_reading);
        delay(2000);
      break;

    case UPDATE_MODE:
      if (request) {
        uint8_t rq[] = {0xF1, (cur + 1) >> 8, (cur + 1) & 0xFF};
        pTxCharacteristic->setValue(rq, 3);
        pTxCharacteristic->notify();
        delay(50);
        request = false;
      }

      if (cur + 1 == parts) {  // Received complete file
        uint8_t com[] = {0xF2, (cur + 1) >> 8, (cur + 1) & 0xFF};
        pTxCharacteristic->setValue(com, 3);
        pTxCharacteristic->notify();
        delay(50);
        MODE = OTA_MODE;
      }

      if (writeFile) {
        if (!current) {
          writeBinary(FLASH, "/update.bin", updater, writeLen);
        } else {
          writeBinary(FLASH, "/update.bin", updater2, writeLen2);
        }
      }
      break;

    case OTA_MODE:
      if (writeFile) {
        if (!current) {
          writeBinary(FLASH, "/update.bin", updater, writeLen);
        } else {
          writeBinary(FLASH, "/update.bin", updater2, writeLen2);
        }
      }

      if (rParts == tParts) {
        Serial.println("OTA Update Complete");
        delay(5000);
        updateFromFS(FLASH);
      } else {
        writeFile = true;
        Serial.println("Incomplete OTA Update");
        Serial.print("Expected: ");
        Serial.print(tParts);
        Serial.print(" Received: ");
        Serial.println(rParts);
        delay(1000);//delay(2000);
      }
      break;
  }
}
