// DeviceApp.h
#pragma once
#include <app/App.h>
#include "config/DeviceConfigManager.h"
#include "config/DeviceDefs.h"
#include "handlers/RelayHandler.h"
// #include "handlers/SensorHandler.h"
#include "handlers/SleepableSensorHandler.h"
#include "handlers/DS18B20Handler.h"

// Constructed before App — ensures config is valid when App initializes
struct DeviceConfig {
    EventBus            eventBus;
    DeviceConfigManager configManager;

    DeviceConfig();  // constructs eventBus + configManager
};

class DeviceApp : public DeviceConfig, public App {
public:
    DeviceApp();
    void begin();

private:
    RelayHandler  _relay;
    // SensorHandler _sensor;
    SleepableSensorHandler _sleepableSensor;
    DS18B20Handler _tempSensors;
};