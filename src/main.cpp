#include <Arduino.h>
#include "ArduinoJson.h"
#include "NimBLEDevice.h"

NimBLEScan* pBLEScan;

JsonDocument bleScanData;
JsonDocument sensorData;

static constexpr uint32_t scanTime_ms = 10000; 

class scanCallbacks : public NimBLEScanCallbacks 
{
  void onDiscovered(const NimBLEAdvertisedDevice* advertisedDevice) override { }

  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override 
  {
    JsonObject BLEdata = bleScanData.to<JsonObject>();
  
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

    std::string serviceData = BLEdata["servicedata"];

    if(BLEdata["servicedatauuid"] == "0xfcd2" && (serviceData.length() == 20 || serviceData.length() == 24))
    {
      JsonObject shellyHT = sensorData.to<JsonObject>();

      shellyHT["mac"] = mac_adress;
      shellyHT["rssi"] = (int)advertisedDevice->getRSSI();
      shellyHT["name"] = "ShellyBLU H&T";
      shellyHT["model"] = (char*)advertisedDevice->getName().c_str();
      shellyHT["packet"] = (uint8_t)strtoul(serviceData.substr(4, 2).c_str(), NULL, 16);
      shellyHT["battery"] = (uint8_t)strtoul(serviceData.substr(8, 2).c_str(), NULL, 16);
      shellyHT["humidity"] = (uint8_t)strtoul(serviceData.substr(12, 2).c_str(), NULL, 16);

      uint8_t lowByte = (uint8_t)strtoul(serviceData.substr(serviceData.length()-4, 2).c_str(), NULL, 16);
      uint8_t highByte = (uint8_t)strtoul(serviceData.substr(serviceData.length()-2, 2).c_str(), NULL, 16);
      uint8_t bytes[] = {lowByte, highByte};
      int16_t temp_c = (int16_t)strtol(String(*(int16_t*)bytes).c_str(), NULL, 10);
      shellyHT["temp_c"] = (float)temp_c/10;
      shellyHT["temp_f"] = (temp_c * 18 + 3200) / (float)100;
      
      if(serviceData.length() == 24 && serviceData.substr(16, 4) == "0145") shellyHT["button"] = 1;
      else if (serviceData.length() == 24 && serviceData.substr(16, 4) == "fe45") shellyHT["button"] = 2;
      else shellyHT["button"] = 0;

      shellyHT["service_data"] = serviceData;

      serializeJson(shellyHT, Serial);
      Serial.println();
    }
    else
    {
      /* Uncomment to print data to serial from other deivces */

      /*
        serializeJson(BLEdata, Serial);
        Serial.println();
      */
    }
  }

  void onScanEnd(const NimBLEScanResults& results, int reason) override 
  {
      NimBLEDevice::getScan()->start(scanTime_ms, false, true);
  }

} scanCallbacks;

void setup() 
{  
  NimBLEDevice::init("ESP32-Scanner");    
  NimBLEScan* pBLEScan = NimBLEDevice::getScan(); 

/* 
    Obtaining data from Shelly BLUE H&T requres active scanning.
    It is recommended to whitelist our sensors so we dont request
    scan responses from other devices to reduce taffic.
*/
/*
  NimBLEAddress sensor1("AA:BB:CC:DD:EE:FF", BLE_ADDR_PUBLIC);
  NimBLEDevice::whiteListAdd(sensor1);
  
  pBLEScan->setFilterPolicy(BLE_HCI_SCAN_FILT_USE_WL);
*/

  pBLEScan->setScanCallbacks(&scanCallbacks, false);
  pBLEScan->setActiveScan(true);         
  pBLEScan->setMaxResults(0);            
  pBLEScan->start(scanTime_ms, false, true);

  Serial.begin(115200);

  printf("Scanning started..\n");
}

void loop() 
{
  vTaskDelay(5000);
}
