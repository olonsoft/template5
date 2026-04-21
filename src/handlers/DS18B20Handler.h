#pragma once

#include <Arduino.h>
#include <vector>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <core/AppHandler.h>
#include <core/ISleepable.h>
#include <core/EventBus.h>
#include <ha/ManageHa.h>
#include "../config/DeviceDefs.h"      // ← device specific, for SENSOR_INTERVAL

namespace DS18B20Config {
    constexpr uint8_t  RESOLUTION  = 12;    // 9, 10, 11 or 12 bit
    constexpr bool     USE_CELSIUS = true;
    constexpr uint8_t  MAX_SENSORS = 10;    // safety cap for bus scan
}

class DS18B20Handler : public AppHandler, public ISleepable {
public:
    DS18B20Handler(EventBus& bus, ManageHa& ha,
                   uint8_t pin,
                   uint32_t interval = DeviceDefaults::SENSOR_INTERVAL);

    void begin() override;
    void loop()  override;

    // ISleepable — publish final reading before sleep
    void onSleep()           override;
    bool isSleepDone() const override { return true; }

private:
    struct Sensor {
        DeviceAddress address;
        String        shortId;   // last 3 unique bytes hex lowercase e.g. "ab5600"
        String        fullId;    // full 8 bytes hex lowercase e.g. "28ff1234ab5600cd"
    };

    void   scanBus();
    void   readAndPublish();
    void   publishDiscovery();
    String addressToShortId(const DeviceAddress& addr) const;
    String addressToFullId(const DeviceAddress& addr)  const;
    bool   isValidReading(float temp)                  const;

    EventBus&         _eventBus;
    ManageHa&         _ha;
    OneWire           _oneWire;
    DallasTemperature _sensors;

    std::vector<Sensor> _found;

    uint32_t _interval;
    uint32_t _lastRead  = 0;
    bool     _mqttReady = false;
};