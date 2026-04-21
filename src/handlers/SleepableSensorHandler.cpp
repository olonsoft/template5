#include "SleepableSensorHandler.h"
#include <core/Logger.h>
#include "../config/DeviceDefs.h"

const char* const TAG = "SleepableSensor";

// ─────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────
SleepableSensorHandler::SleepableSensorHandler(EventBus& bus, ManageHa& ha,
                             ManageSleep& sleep, uint8_t pin)
    : _eventBus(bus), _ha(ha), _sleep(sleep), _pin(pin)
{}

void SleepableSensorHandler::begin() {
    _sleep.registerParticipant("sensor", 1, *this);  // phase 1 — before mqtt and wifi
    // _sleep.registerParticipant("sensor", 0, *this);  // phase 0 — before everything
    // ... rest of begin()
}

void SleepableSensorHandler::loop() {
    // ... normal loop code, e.g. reading sensor and publishing to MQTT
}

void SleepableSensorHandler::onSleep() {
    // Turn off heater, stop readings, whatever cleanup needed
    olog.info(TAG, "Sleep — shutting down sensor");
}