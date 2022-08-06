#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <iarduino_DHT.h> 


// #define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
// #define TIME_TO_SLEEP  10        /* Time ESP32 will go to sleep (in seconds) */
#define SERVICE_UUID        "8aa816be-b599-4696-a2d6-c4f07b887f28"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

iarduino_DHT sensor(23);  // DTT11 PIN
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

void dhtREAD(){                 
  Serial.print  ("DHT11 readings: ");
  switch(sensor.read()){ 
    case DHT_OK:             Serial.println((String) sensor.hum + "% - " + sensor.tem + "*C"); break;
    case DHT_ERROR_CHECKSUM: Serial.println("ERROR_CHECKSUM");                                 break;
    case DHT_ERROR_DATA:     Serial.println("Not a DHT-type responce");                        break;
    case DHT_ERROR_NO_REPLY: Serial.println("No reply from the sensor");                       break;
    default:                 Serial.println("NaN");                                            break;
  } 
}


void setup() {
  Serial.begin(9600);

 // Read Sensor Data for the 1st time, after that read it with delay
  dhtREAD();

  // создаем BLE-устройство:
  BLEDevice::init("AnotherMeteo_BLE");

  // Создаем BLE-сервер:
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Создаем BLE-сервис:
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Создаем BLE-характеристику:
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // создаем BLE-дескриптор:
  pCharacteristic->addDescriptor(new BLE2902());

  // запускаем сервис:
  pService->start();

  // запускаем оповещения (advertising):
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");  

  // esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  // Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

}

static char tempStr[16];
static char humStr[16];

unsigned long timeSensorUpdatedLast = 0;

void loop() {


  // Read Sensor every 10s
  if ((millis()) > (timeSensorUpdatedLast + 10000))
  {
    dhtREAD();  
    // sensor.read();
    timeSensorUpdatedLast = millis();

  }

  if (deviceConnected) 
  {
    // ToStringing routine
    dtostrf(sensor.tem, 4, 1, tempStr);
    dtostrf(sensor.hum, 4, 1, humStr);

    // Setting value to the characteristic
    pCharacteristic->setValue(tempStr);
    pCharacteristic->notify();

  }

  delay(3000);

  // Serial.flush(); 
  // esp_deep_sleep_start();

}
