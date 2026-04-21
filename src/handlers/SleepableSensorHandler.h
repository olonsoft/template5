#pragma once

#include <Arduino.h>
#include <core/AppHandler.h>
#include <core/EventBus.h>
#include <ha/ManageHa.h>
#include <sleep/ManageSleep.h>

// SensorHandler.h
class SleepableSensorHandler : public AppHandler, public ISleepable {
public:
    SleepableSensorHandler(EventBus& bus, ManageHa& ha, ManageSleep& sleep, uint8_t pin);

    void begin()          override;
    void loop()           override;

    // ISleepable
    void onSleep()           override;
    bool isSleepDone() const override { return true; }  // synchronous

private:
    EventBus& _eventBus;
    ManageHa& _ha;
    ManageSleep& _sleep;
    uint8_t _pin;
    // ..
};