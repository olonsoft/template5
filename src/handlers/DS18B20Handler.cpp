#include "DS18B20Handler.h"

#include <ArduinoJson.h>
#include <core/Logger.h>
#include <utils/StringUtils.h>

const char* const TAG = "DS18B20";

// ─────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────
DS18B20Handler::DS18B20Handler(EventBus& bus, ManageHa& ha, uint8_t pin, uint32_t interval)
    : _eventBus(bus), _ha(ha), _oneWire(pin), _sensors(&_oneWire), _interval(interval) {
  _eventBus.subscribe(EventType::APP_MQTT_CLIENT_CONNECTED, [this](EventType, const void*) {
    _mqttReady = true;
    readAndPublish();   // immediate reading on every connect
  });

  _eventBus.subscribe(EventType::APP_MQTT_CLIENT_DISCONNECTED, [this](EventType, const void*) {
    _mqttReady = false;
  });
}

// ─────────────────────────────────────────────
// AppHandler
// ─────────────────────────────────────────────
void DS18B20Handler::begin() {
  _sensors.begin();
  _sensors.setResolution(DS18B20Config::RESOLUTION);
  scanBus();
  publishDiscovery();   // register entities with ManageHa once at startup
                          // ManageHa publishes discovery on every MQTT connect
}

void DS18B20Handler::loop() {
  if (!_mqttReady) return;

  uint32_t now = millis();
  if (_lastRead != 0 && now - _lastRead < _interval) return;
  _lastRead = now;

  readAndPublish();
}

// ─────────────────────────────────────────────
// ISleepable
// ─────────────────────────────────────────────
void DS18B20Handler::onSleep() {
  olog.info(TAG, "Sleep — publishing final readings");
  readAndPublish();
}

// ─────────────────────────────────────────────
// Private
// ─────────────────────────────────────────────
void DS18B20Handler::scanBus() {
  _found.clear();

  uint8_t count = _sensors.getDeviceCount();
  if (count == 0) {
    olog.warn(TAG, "No sensors found on bus");
    return;
  }

  count = min(count, (uint8_t)DS18B20Config::MAX_SENSORS);
  olog.info(TAG, "Found %d sensor(s)", count);

  for (uint8_t i = 0; i < count; i++) {
    Sensor s;
    if (!_sensors.getAddress(s.address, i)) continue;
    s.shortId = addressToShortId(s.address);
    s.fullId = addressToFullId(s.address);
    _found.push_back(s);
    olog.info(TAG, "  [%d] %s", i, s.fullId.c_str());
  }
}

void DS18B20Handler::readAndPublish() {
  if (_found.empty() || !_mqttReady) return;

  _sensors.requestTemperatures();  // blocking ~750ms at 12-bit

  JsonDocument doc;
  bool anyValid = false;

  for (const auto& sensor : _found) {
    float temp = DS18B20Config::USE_CELSIUS ? _sensors.getTempC(sensor.address) : _sensors.getTempF(sensor.address);

    if (!isValidReading(temp)) {
      String msg = "Bad reading from " + sensor.shortId;
      AppError err{ TAG, msg.c_str() };
      olog.warn(TAG, "Skipping %s — bad reading (%.1f)", sensor.shortId.c_str(), temp);
      _eventBus.publish(EventType::APP_ERROR_RECOVERABLE, &err);
      continue;
    }

    float rounded = roundf(temp * 100.0f) / 100.0f;
    doc[sensor.fullId] = rounded;
    anyValid = true;

    olog.info(TAG, "%s → %.2f%s", sensor.shortId.c_str(), rounded, DS18B20Config::USE_CELSIUS ? "°C" : "°F");
  }

  if (!anyValid) {
    olog.warn(TAG, "No valid readings — skipping publish");
    return;
  }

  _ha.publishJson("temperatures", doc, false);
}

void DS18B20Handler::publishDiscovery() {
  if (_found.empty()) return;

    // Get resolved base topic from ManageHa
  String stateTopic = StringUtils::joinTopic(_ha.getBaseTopic(), "temperatures");

  for (const auto& sensor : _found) {
    String valueTemplate = "{{ value_json['" + sensor.fullId + "'] }}";
    String name = "Temperature " + sensor.shortId;
    String id = "temp_" + sensor.shortId;

    _ha.addSensorJson(id,
                      name,
                      stateTopic,
                      valueTemplate,
                      DS18B20Config::USE_CELSIUS ? "°C" : "°F",
                      "temperature",
                      "mdi:thermometer");

    olog.info(TAG, "Registered HA entity: %s → %s", id.c_str(), stateTopic.c_str());
  }
}

String DS18B20Handler::addressToShortId(const DeviceAddress& addr) const {
  char buf[7];
  // Use last 3 bytes which are usually unique, and skip family code (first byte) which is always 0x28 for DS18B20
  // The last byte is the CRC
  snprintf(buf, sizeof(buf), "%02x%02x%02x", addr[1], addr[2], addr[3]);
  return String(buf);
}

String DS18B20Handler::addressToFullId(const DeviceAddress& addr) const {
  char buf[17];
  snprintf(buf,
           sizeof(buf),
           "%02x%02x%02x%02x%02x%02x%02x%02x",
           addr[0],
           addr[1],
           addr[2],
           addr[3],
           addr[4],
           addr[5],
           addr[6],
           addr[7]);
  return String(buf);
}

bool DS18B20Handler::isValidReading(float temp) const {
  if (temp == DEVICE_DISCONNECTED_C) return false;
  if (temp == 85.0f) return false;
  if (isnan(temp)) return false;
  return true;
}