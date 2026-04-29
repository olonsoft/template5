#include "RelayHandler.h"

#include <core/Logger.h>
#include <mqtt/ManageMQTT.h>

const char* const TAG = "Relay";

RelayHandler::RelayHandler(EventBus& bus, ManageHa& ha, uint8_t pin) : _eventBus(bus), _ha(ha), _pin(pin) {}

void RelayHandler::begin() {
  pinMode(_pin, OUTPUT);
  setRelay(false);

    // Register HA entity
  _ha.addSwitch("relay", "Relay", "mdi:power");

    // Handle MQTT commands
  _eventBus.subscribe(EventType::APP_MQTT_MESSAGE_RECEIVED, [this](EventType, const void* p) {
    const MqttMessage* msg = static_cast<const MqttMessage*>(p);
    String topic(msg->topic);

    if (topic.endsWith("/cmd/relay")) {
      setRelay(String(msg->payload) == "ON");
    }
  });

    // Publish initial state when MQTT connects
  _eventBus.subscribe(EventType::APP_MQTT_CLIENT_CONNECTED, [this](EventType, const void*) {
    _ha.publishState("relay", _state ? "ON" : "OFF");
  });

  olog.info(TAG, "Relay handler started on pin %d", _pin);
}

void RelayHandler::loop() {
    // Nothing needed — event driven
}

void RelayHandler::setRelay(bool on) {
  _state = on;
  digitalWrite(_pin, on ? HIGH : LOW);
  _ha.publishState("relay", on ? "ON" : "OFF");
  olog.info(TAG, "Relay → %s", on ? "ON" : "OFF");
}