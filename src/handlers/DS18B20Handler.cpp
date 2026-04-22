#include "DS18B20Handler.h"

#include <ArduinoJson.h>
#include <core/Logger.h>
#include <utils/StringUtils.h>

const char* const TAG = "DS18B20";

// ─────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────
DS18B20Handler::DS18B20Handler(EventBus& bus, ManageHa& ha, ManageSleep& sleep, uint8_t pin, uint32_t interval)
    : _eventBus(bus), _ha(ha), _sleep(sleep), _oneWire(pin), _sensors(&_oneWire), _interval(interval) {
  _eventBus.subscribe(EventType::APP_MQTT_CLIENT_CONNECTED, [this](EventType, const void*) {
    _mqttReady = true;
    startConversion(ReadState::PERIODIC_WAITING);
  });

  _eventBus.subscribe(EventType::APP_MQTT_CLIENT_DISCONNECTED, [this](EventType, const void*) {
    _mqttReady = false;
  });
}

// ─────────────────────────────────────────────
// AppHandler
// ─────────────────────────────────────────────
void DS18B20Handler::begin() {
  _sleep.registerParticipant("ds18b20", 1, *this);
  _sensors.begin();
  _sensors.setResolution(DS18B20Config::RESOLUTION);
  _sensors.setWaitForConversion(false);

  switch (DS18B20Config::RESOLUTION) {
    case 9:  _conversionTime = 94; break;
    case 10: _conversionTime = 188; break;
    case 11: _conversionTime = 375; break;
    case 12: _conversionTime = 750; break;
  }

  scanBus();
  publishDiscovery();
}

void DS18B20Handler::loop() {
  if (!_mqttReady) return;

  uint32_t now = millis();

  switch (_state) {
    case ReadState::IDLE:
      if (_lastRead == 0 || now - _lastRead >= _interval) {
        startConversion(ReadState::PERIODIC_WAITING);
      }
      break;

    case ReadState::PERIODIC_WAITING:
    case ReadState::SLEEP_WAITING:
      if (now - _conversionStart >= _conversionTime) {
        finishConversion();
      }
      break;
  }
}

// ─────────────────────────────────────────────
// Sleep handling
// ─────────────────────────────────────────────
void DS18B20Handler::onSleep() {
  olog.info(TAG, "Sleep — forcing final async read");

  if (_found.empty() || !_mqttReady) {
    _state = ReadState::IDLE;
    return;
  }

  startConversion(ReadState::SLEEP_WAITING);
}

bool DS18B20Handler::isSleepDone() const {
  return _state != ReadState::SLEEP_WAITING;
}

// ─────────────────────────────────────────────
// Async state machine
// ─────────────────────────────────────────────
void DS18B20Handler::startConversion(ReadState nextState) {
  if (_found.empty()) {
    _state = ReadState::IDLE;
    return;
  }

  _lastRead = millis();
  _conversionStart = millis();
  _state = nextState;

  olog.debug(TAG,
             "Starting async temperature conversion (%s)",
             nextState == ReadState::SLEEP_WAITING ? "sleep" : "periodic");

  _sensors.requestTemperatures();
}

void DS18B20Handler::finishConversion() {
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

  if (anyValid) {
    _ha.publishJson("temperatures", doc, false);
  } else {
    olog.warn(TAG, "No valid readings — skipping publish");
  }

  _state = ReadState::IDLE;
}

// ─────────────────────────────────────────────
// Bus scan, discovery, helpers
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

void DS18B20Handler::publishDiscovery() {
  if (_found.empty()) return;

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
