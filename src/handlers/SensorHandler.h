#pragma once

#include <Arduino.h>
#include <core/AppHandler.h>
#include <core/EventBus.h>
#include <ha/ManageHa.h>

// Swap DHT for your actual sensor library
#include <DHT.h>

class SensorHandler : public AppHandler {
public:
    SensorHandler(EventBus& bus, ManageHa& ha,
                  uint8_t pin, uint8_t dhtType = DHT22);

    void begin() override;
    void loop()  override;

private:
    void readAndPublish();

    EventBus& _eventBus;
    ManageHa& _ha;
    DHT       _dht;

    uint32_t _lastRead    = 0;
    bool     _mqttReady   = false;
};