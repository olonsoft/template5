#pragma once
#include <config/FrameworkConfigManager.h>
#include "DeviceDefs.h"

class DeviceConfigManager : public FrameworkConfigManager {
public:
    DeviceConfigManager(EventBus& bus, const FrameworkConfig& config)  // ← added constructor
        : FrameworkConfigManager(bus, config)
        , _sensorInterval(DeviceDefaults::SENSOR_INTERVAL)             // ← init from DeviceDefaults
    {}

    uint32_t sensorInterval() const { return _sensorInterval; }

    void setSensorInterval(uint32_t v) {
        if (_sensorInterval == v) return;
        _sensorInterval = v;
        publishChange();   // ← notify about the change
    }

protected:
    void loadExtended(Preferences& prefs) override {
        _sensorInterval = prefs.getUInt("sensorInterval", DeviceDefaults::SENSOR_INTERVAL); // ← load with default fallback
    }

    void saveExtended(Preferences& prefs) override {
        prefs.putUInt("sensorInterval", _sensorInterval);
    }

private:
    uint32_t _sensorInterval;
};