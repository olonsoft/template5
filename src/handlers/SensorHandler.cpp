#include "SensorHandler.h"

#include <core/Logger.h>

#include "../config/DeviceDefs.h"

const char* const TAG = "Sensor";

// ─────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────
SensorHandler::SensorHandler(EventBus& bus, ManageHa& ha, uint8_t pin, uint8_t dhtType)
    : _eventBus(bus), _ha(ha), _dht(pin, dhtType) {}

// ─────────────────────────────────────────────
// AppHandler
// ─────────────────────────────────────────────
void SensorHandler::begin() {
  _dht.begin();

    // Register HA entities
  _ha.addSensor("temperature", "Temperature", "°C", "temperature", "mdi:thermometer");
  _ha.addSensor("humidity", "Humidity", "%", "humidity", "mdi:water-percent");

    // Publish initial reading when MQTT connects
  _eventBus.subscribe(EventType::APP_MQTT_CLIENT_CONNECTED, [this](EventType, const void*) {
    _mqttReady = true;
    readAndPublish();
  });

  _eventBus.subscribe(EventType::APP_MQTT_CLIENT_DISCONNECTED, [this](EventType, const void*) {
    _mqttReady = false;
  });

  olog.info(TAG, "Sensor handler started — interval %ds", DeviceDefaults::SENSOR_INTERVAL / 1000);
}

void SensorHandler::loop() {
  if (!_mqttReady) return;

  uint32_t now = millis();
  if (_lastRead != 0 && now - _lastRead < DeviceDefaults::SENSOR_INTERVAL) return;
  _lastRead = now;

  readAndPublish();
}

// ─────────────────────────────────────────────
// Private
// ─────────────────────────────────────────────
void SensorHandler::readAndPublish() {
  float temperature = _dht.readTemperature();
  float humidity = _dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    AppError err{ "Sensor", "Failed to read DHT sensor" };
    olog.error(TAG, "%s", err.message);
    _eventBus.publish(EventType::APP_ERROR_RECOVERABLE, &err);
    return;
  }

  olog.info(TAG, "Temperature: %.1f°C  Humidity: %.1f%%", temperature, humidity);

  _ha.publishState("temperature", String(temperature, 1));
  _ha.publishState("humidity", String(humidity, 1));
}