#pragma once

#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <core/AppHandler.h>
#include <core/EventBus.h>
#include <core/ISleepable.h>
#include <ha/ManageHa.h>
#include <sleep/ManageSleep.h>

#include <vector>

#include "../config/DeviceDefs.h"

namespace DS18B20Config {
  constexpr uint8_t RESOLUTION = 12;
  constexpr bool USE_CELSIUS = true;
  constexpr uint8_t MAX_SENSORS = 10;
} // namespace DS18B20Config

using ReadCompleteCallback = std::function<void(bool success, size_t sensorCount)>;

class DS18B20Handler : public AppHandler, public ISleepable {
 public:
  DS18B20Handler(EventBus& bus, ManageHa& ha, ManageSleep& sleep, uint8_t pin,
                 uint32_t interval = DeviceDefaults::SENSOR_INTERVAL);

  void begin() override;
  void loop() override;

  void onSleep() override;
  bool isSleepDone() const override;
  void setReadCompleteCallback(ReadCompleteCallback cb) { _readCompleteCallback = cb; }

 private:
  struct Sensor {
    DeviceAddress address;
    String shortId;
    String fullId;
  };

  enum class ReadState {
    IDLE,
    PERIODIC_WAITING,
    SLEEP_WAITING
  };

  void scanBus();
  void publishDiscovery();
  void startConversion(ReadState nextState);
  void finishConversion();
  bool isRecent(uint32_t now) const;

  String addressToShortId(const DeviceAddress& addr) const;
  String addressToFullId(const DeviceAddress& addr) const;
  bool isValidReading(float temp) const;

  EventBus& _eventBus;
  ManageHa& _ha;
  ManageSleep& _sleep;
  OneWire _oneWire;
  DallasTemperature _sensors;

  std::vector<Sensor> _found;

  ReadCompleteCallback _readCompleteCallback = nullptr;
    uint32_t _interval;
  uint32_t _lastRead = 0;
  bool _mqttReady = false;

  ReadState _state = ReadState::IDLE;
  uint32_t _conversionStart = 0;
  uint16_t _conversionTime = 750;
};
