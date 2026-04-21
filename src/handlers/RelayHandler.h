#pragma once

#include <Arduino.h>
#include <core/AppHandler.h>
#include <core/EventBus.h>
#include <ha/ManageHa.h>
#include <mqtt/ManageMQTT.h>

class RelayHandler : public AppHandler {
public:
    RelayHandler(EventBus& bus, ManageHa& ha, uint8_t pin);

    void begin() override;
    void loop()  override;

private:
    void setRelay(bool on);

    EventBus& _eventBus;
    ManageHa& _ha;
    uint8_t   _pin;
    bool      _state = false;
};