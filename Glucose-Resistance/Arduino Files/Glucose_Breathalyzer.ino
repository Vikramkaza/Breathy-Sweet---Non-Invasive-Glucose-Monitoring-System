#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include "SH1106Wire.h"
#include <MovingAverage.h>

#define SENSOR1_PIN 39  //MP905
#define SENSOR2_PIN 34  //MP905
#define SENSOR3_PIN 36  //MP901
#define BUTTON_PIN 13
#define GREENLED_1 32
#define GREENLED_2 33
#define GREENLED_3 25
#define GREENLED_4 26
#define GREENLED_5 27
#define ORANGELED_1 14

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

SH1106Wire display(0x3c, SDA, SCL);

MovingAverage<uint16_t, 8> Sensor1Filter;
MovingAverage<uint16_t, 8> Sensor2Filter;
MovingAverage<uint16_t, 8> Sensor3Filter;

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;
unsigned long pBLEMillis = 0;
uint16_t BLEInterval = 250;

uint16_t Sensor1_Data;
uint16_t Sensor2_Data;
uint16_t Sensor3_Data;

uint32_t glucoseInt;
float randF = 4.75;
String temp_result;
uint8_t startBit = 0;
uint8_t stateCount = 0;
int count;
bool prevPressed = 0;
uint8_t screenBlinker;
uint8_t resultCounter;
bool resultSent;

String glucoseValue;

const unsigned char ble_icon[] PROGMEM = {
  0x04, 0x00, 0x0c, 0x00, 0x15, 0x00, 0x26, 0x00, 0x14, 0x00, 0x0c, 0x00, 0x14, 0x00, 0x26, 0x00,
  0x15, 0x00, 0x0c, 0x00, 0x04, 0x00
};


class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
  void onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    glucoseValue = "";
    if (value.length() > 0) {
      for (int i = 0; i < value.length(); i++) {
        glucoseValue.concat(value[i]);
      }
      glucoseInt = glucoseValue.toInt();
      glucoseValue = String(glucoseInt);
    }
  }
};

void bleTransmit() {
  char TxBuffer[32];
  sprintf(TxBuffer, "%d, %d, %d, %d\n", Sensor1_Data, Sensor2_Data, Sensor3_Data, glucoseInt);
  if (deviceConnected) {
    pCharacteristic->setValue((char*)&TxBuffer);
    pCharacteristic->notify();
  }
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
}

void setup() {
  pinMode(SENSOR1_PIN, INPUT);
  pinMode(SENSOR2_PIN, INPUT);
  pinMode(SENSOR3_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(GREENLED_1, OUTPUT);
  pinMode(GREENLED_2, OUTPUT);
  pinMode(GREENLED_3, OUTPUT);
  pinMode(GREENLED_4, OUTPUT);
  pinMode(GREENLED_5, OUTPUT);
  pinMode(ORANGELED_1, OUTPUT);
  Serial.begin(115200);

  //Display Init
  display.init();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.setColor(WHITE);
  // Uncomment to Flip Screen
  // display.flipScreenVertically();
  delay(250);

  BLEDevice::init("Glucose Breathalyzer");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService* pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();

  for (int count = 20; count > 0; count--) {
    if (digitalRead(BUTTON_PIN) == 0) {
      display.clear();
      display.drawString(30, 10, "Skipping");
      display.drawString(30, 35, "Prewarm");
      display.display();
      delay(1000);
      break;
    }
    display.clear();
    display.drawString(20, 10, "Initializing...");
    display.drawString(52, 35, String(count));
    delay(1000);
    display.display();
  }

  display.clear();
  display.drawString(25, 10, "Press Button");
  display.drawString(45, 35, "to Start");
  if (deviceConnected) {
    display.drawXbm(115, 3, 10, 11, ble_icon);
  }
  display.display();
}

void loop() {
  Sensor1_Data = Sensor1Filter.add(analogRead(SENSOR1_PIN));
  Sensor2_Data = Sensor2Filter.add(analogRead(SENSOR2_PIN));
  Sensor3_Data = Sensor3Filter.add(analogRead(SENSOR3_PIN));


  if (digitalRead(BUTTON_PIN) == 0) {
    if (!prevPressed) {
      startBit = 1;
      stateCount = 0;
      count = 0;
      prevPressed = 1;
      display.clear();
      screenBlinker = 0;
      resultCounter = 0;
      // glucoseValue = "";
      // glucoseInt = 0;
      resultSent = false;
      temp_result = "";
      randF = 4.75;
    }
  } else {
    prevPressed = 0;
  }

  if (startBit) {
    count += 1;
    if (count >= 10) {
      if (stateCount < 7) {
        stateCount += 1;
      } else {
        stateCount = 7;
      }
      count = 0;
    }

    switch (stateCount) {

      case 0:
        digitalWrite(GREENLED_1, HIGH);
        digitalWrite(GREENLED_2, HIGH);
        digitalWrite(GREENLED_3, HIGH);
        digitalWrite(GREENLED_4, HIGH);
        digitalWrite(GREENLED_5, HIGH);
        digitalWrite(ORANGELED_1, LOW);
        display.clear();
        display.drawString(12, 20, "Calculating...");
        if (deviceConnected) {
          display.drawXbm(115, 3, 10, 11, ble_icon);
        }
        display.display();
        break;

      case 1:
        digitalWrite(GREENLED_1, LOW);
        digitalWrite(GREENLED_2, HIGH);
        digitalWrite(GREENLED_3, HIGH);
        digitalWrite(GREENLED_4, HIGH);
        digitalWrite(GREENLED_5, HIGH);
        digitalWrite(ORANGELED_1, LOW);
        break;

      case 2:
        digitalWrite(GREENLED_1, LOW);
        digitalWrite(GREENLED_2, LOW);
        digitalWrite(GREENLED_3, HIGH);
        digitalWrite(GREENLED_4, HIGH);
        digitalWrite(GREENLED_5, HIGH);
        digitalWrite(ORANGELED_1, LOW);
        break;

      case 3:
        digitalWrite(GREENLED_1, LOW);
        digitalWrite(GREENLED_2, LOW);
        digitalWrite(GREENLED_3, LOW);
        digitalWrite(GREENLED_4, HIGH);
        digitalWrite(GREENLED_5, HIGH);
        digitalWrite(ORANGELED_1, LOW);
        break;

      case 4:
        digitalWrite(GREENLED_1, LOW);
        digitalWrite(GREENLED_2, LOW);
        digitalWrite(GREENLED_3, LOW);
        digitalWrite(GREENLED_4, LOW);
        digitalWrite(GREENLED_5, HIGH);
        digitalWrite(ORANGELED_1, LOW);
        break;

      case 5:
        digitalWrite(GREENLED_1, LOW);
        digitalWrite(GREENLED_2, LOW);
        digitalWrite(GREENLED_3, LOW);
        digitalWrite(GREENLED_4, LOW);
        digitalWrite(GREENLED_5, LOW);
        digitalWrite(ORANGELED_1, HIGH);
        if (!resultSent) {
          Serial.print(String(Sensor1_Data) + "," + String(Sensor2_Data) + "," + String(Sensor3_Data) + "\n");
          resultSent = true;
        }
        display.clear();
        display.drawString(12, 20, "Processing...");
        if (deviceConnected) {
          display.drawXbm(115, 3, 10, 11, ble_icon);
        }
        display.display();
        break;

      case 6:
        display.clear();
        if (randF > 0 && count % 2 == 0) {
          temp_result = String(glucoseInt - (randF * randF));
          randF = randF - 1.07;
        }
        if (deviceConnected) {
          display.drawXbm(115, 3, 10, 11, ble_icon);
        }
        display.drawString(12, 8, "Calculating...");
        display.drawString(34, 33, temp_result);
        display.setFont(ArialMT_Plain_10);
        display.drawString(90, 39, "mg/dL");
        display.setFont(ArialMT_Plain_16);
        display.display();
        break;

      case 7:
        digitalWrite(GREENLED_1, LOW);
        digitalWrite(GREENLED_2, LOW);
        digitalWrite(GREENLED_3, LOW);
        digitalWrite(GREENLED_4, LOW);
        digitalWrite(GREENLED_5, LOW);
        digitalWrite(ORANGELED_1, LOW);
        if (resultCounter < 10) {
          if (screenBlinker >= 10) {
            screenBlinker = 0;
            resultCounter += 1;
          }
          if (screenBlinker <= 6) {
            display.clear();
            display.drawString(8, 8, "Glucose Value:");
            if (glucoseValue.length() == 0) {
              display.drawString(34, 33, "--");
            } else {
              display.drawString(34, 33, glucoseValue);
            }
            display.setFont(ArialMT_Plain_10);
            display.drawString(90, 39, "mg/dL");
            display.setFont(ArialMT_Plain_16);
            if (deviceConnected) {
              display.drawXbm(115, 3, 10, 11, ble_icon);
            }
            display.display();
          } else {
            display.clear();
            display.display();
          }
          screenBlinker += 1;
        } else {
          display.clear();
          display.drawString(20, 10, "Press Button");
          display.drawString(45, 35, "to Start");
          if (deviceConnected) {
            display.drawXbm(115, 3, 10, 11, ble_icon);
          }
          display.display();
        }
        break;

      default:
        digitalWrite(GREENLED_1, LOW);
        digitalWrite(GREENLED_2, LOW);
        digitalWrite(GREENLED_3, LOW);
        digitalWrite(GREENLED_4, LOW);
        digitalWrite(GREENLED_5, LOW);
        digitalWrite(ORANGELED_1, LOW);
    }
  }

  if (Serial.available()) {
    glucoseValue = Serial.readString();
    glucoseInt = glucoseValue.toInt();
  }


  unsigned long cBLEMillis = millis();
  if (cBLEMillis - pBLEMillis >= BLEInterval) {
    bleTransmit();
    pBLEMillis = cBLEMillis;
  }
  delay(250);
}