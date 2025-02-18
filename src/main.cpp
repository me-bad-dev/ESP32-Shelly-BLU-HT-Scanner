#include <Arduino.h>
#include "ArduinoJson.h"
#include "NimBLEDevice.h"
#include "decoder.h"

#define ARDUINOJSON_USE_LONG_LONG 1

NimBLEScan* pBLEScan;

TheengsDecoder decoder;

JsonDocument sensorData;

static constexpr uint32_t scanTimeMs = 10 * 1000; 

class scanCallbacks : public NimBLEScanCallbacks 
{
  void onDiscovered(const NimBLEAdvertisedDevice* advertisedDevice) override { }

  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override 
  {
    JsonObject BLEdata = sensorData.to<JsonObject>();
  
    String mac_adress = advertisedDevice->getAddress().toString().c_str();
    mac_adress.toUpperCase();
    BLEdata["id"] = (char*)mac_adress.c_str();

    if (advertisedDevice->haveName())
    {
      BLEdata["name"] = (char*)advertisedDevice->getName().c_str();
    }

    if (advertisedDevice->haveManufacturerData())
    {
      std::string manufacturerdata = BLEUtils::dataToHexString((uint8_t*)advertisedDevice->getManufacturerData().data(), advertisedDevice->getManufacturerData().length());
      BLEdata["manufacturerdata"] = manufacturerdata;
    }

    BLEdata["rssi"] = (int)advertisedDevice->getRSSI();

    if (advertisedDevice->haveTXPower())
    {
      BLEdata["txpower"] = (int8_t)advertisedDevice->getTXPower();
    }

    if (advertisedDevice->haveServiceData()) 
    {
      int serviceDataCount = advertisedDevice->getServiceDataCount();
      for (int j = 0; j < serviceDataCount; j++) 
      {
        int serviceDataLength = (int)advertisedDevice->getServiceData(j).length();
        char spr[2 * serviceDataLength + 1];
        for (int i = 0; i < serviceDataLength; i++) sprintf(spr + 2 * i, "%.2x", (unsigned char)advertisedDevice->getServiceData(j)[i]);
        spr[2 * serviceDataLength] = 0;

        std::string service_data = spr;
        BLEdata["servicedata"] = (char*)service_data.c_str();

        std::string serviceDatauuid = advertisedDevice->getServiceDataUUID(j).toString();
        BLEdata["servicedatauuid"] = (char*)serviceDatauuid.c_str();
      }
    }
  
    if (decoder.decodeBLEJson(BLEdata)) 
    {
      if(BLEdata["model_id"] == "SBHT-003C") 
      {
        BLEdata.remove("manufacturerdata");
        BLEdata.remove("servicedata");
        BLEdata.remove("servicedatauuid");
        BLEdata.remove("type");
        BLEdata.remove("cidc");
        BLEdata.remove("acts");
        BLEdata.remove("cont");
        BLEdata.remove("track");

        serializeJson(BLEdata, Serial);
        Serial.println();
      }
      else return;
    }
  }

  void onScanEnd(const NimBLEScanResults& results, int reason) override 
  {
      NimBLEDevice::getScan()->start(scanTimeMs, false, true);
  }

} scanCallbacks;

void setup() 
{  
  NimBLEDevice::init("ESP32-Scanner");    
  NimBLEScan* pBLEScan = NimBLEDevice::getScan(); 

  pBLEScan->setScanCallbacks(&scanCallbacks, false);
  pBLEScan->setActiveScan(true);         
  pBLEScan->setMaxResults(0);            
  pBLEScan->start(scanTimeMs, false, true);

  Serial.begin(115200);

  printf("Scanning started..\n");
}

void loop() 
{
  vTaskDelay(5);
}