#include "DS18B20Handler.h"

#include <ArduinoJson.h>
#include <core/Logger.h>
#include <utils/StringUtils.h>

const char* const TAG = "DS18B20";

DS18B20Handler::DS18B20Handler(EventBus& bus, ManageHa& ha, ManageSleep& sleep, uint8_t pin, uint32_t interval)
    : _eventBus(bus), _ha(ha), _sleep(sleep), _oneWire(pin), _sensors(&_oneWire), _interval(interval) {
  _eventBus.subscribe(EventType::APP_MQTT_CLIENT_CONNECTED, [this](EventType, const void*) {
    _mqttReady = true;
  });

  _eventBus.subscribe(EventType::APP_MQTT_CLIENT_DISCONNECTED, [this](EventType, const void*) {
    _mqttReady = false;
  });
}

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

    // Do not start new periodic reads once sleep is requested
  if (_sleepRequested && _state == ReadState::IDLE) return;

  switch (_state) {
    case ReadState::IDLE:
      if (!_sleepRequested && (_lastRead == 0 || now - _lastRead >= _interval)) {
        _lastRead = now;
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

void DS18B20Handler::onSleep() {
  _sleepRequested = true;

  uint32_t now = millis();

    // If a periodic read is already in progress, wait for it
  if (_state == ReadState::PERIODIC_WAITING) {
    olog.info(TAG, "Sleep — waiting for ongoing read to finish");
    return;
  }

    // If last read is fresh, skip final read
  if (_state == ReadState::IDLE && isRecent(now)) {
    olog.debug(TAG, "Sleep — last reading is fresh, skipping final read");
    return;
  }

  if (_found.empty() || !_mqttReady) {
    _state = ReadState::IDLE;
    return;
  }

  olog.info(TAG, "Sleep — forcing final async read");
  startConversion(ReadState::SLEEP_WAITING);
}

bool DS18B20Handler::isSleepDone() const {
  return _state == ReadState::IDLE;
}

bool DS18B20Handler::isRecent(uint32_t now) const {
  return _lastRead != 0 && (now - _lastRead) < 2000;
}

void DS18B20Handler::startConversion(ReadState nextState) {
  if (_found.empty()) {
    _state = ReadState::IDLE;
    return;
  }

  _state = nextState;
  _conversionStart = millis();

  olog.debug(TAG,
             "Starting async broadcast conversion (%s)",
             nextState == ReadState::SLEEP_WAITING ? "sleep" : "periodic");

  _sensors.requestTemperatures();
}

void DS18B20Handler::finishConversion() {
  bool isPeriodic = (_state == ReadState::PERIODIC_WAITING);

  JsonDocument doc;
  bool anyValid = false;

  for (const auto& sensor : _found) {
    float temp = DS18B20Config::USE_CELSIUS ? _sensors.getTempC(sensor.address) : _sensors.getTempF(sensor.address);

    if (!isValidReading(temp)) continue;

    float rounded = roundf(temp * 100.0f) / 100.0f;
    doc[sensor.fullId] = rounded;
    anyValid = true;

    olog.info(TAG, "%s → %.2f%s", sensor.shortId.c_str(), rounded, DS18B20Config::USE_CELSIUS ? "°C" : "°F");
  }

  _state = ReadState::IDLE;
  _lastRead = millis();

  if (anyValid)
    _ha.publishJson("temperatures", doc, false);
  else
    olog.warn(TAG, "No valid readings — skipping publish");

    // DS18B20-triggered sleep
  if (isPeriodic && _sleepAfterRead && anyValid && !_sleepRequested) {
    olog.info(TAG, "Read complete — triggering sleep");
    _sleepRequested = true;
    SleepRequest req(_sleepDurationUs);
    _eventBus.publish(EventType::APP_SYSTEM_SLEEP_PREPARING, &req);
  }
}

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
