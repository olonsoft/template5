#pragma once
#include <Arduino.h>
#include <config/FrameworkDefs.h>
#include <core/EventBus.h>

namespace DeviceDefaults {
// Device identity
  constexpr const char* APP_NAME = "TemperatureSensor";
  constexpr const char* APP_VERSION = "1.0.0";
  constexpr const char* APP_AUTHOR = "Dimitris";

// Overrides
  constexpr const char* HOSTNAME = "esp-temp-$mac";
  constexpr const char* WIFI_AP_PASSWORD = "12345678";
  constexpr const char* MQTT_CLIENT_ID = "$mac";
  constexpr const char* BASE_TOPIC = "olon/device-$mac";
  constexpr const char* OTA_URL = "http://example.com/temp-sensor.bin";
  constexpr const char* TIME_SERVER = "time.ics.forth.gr";
  constexpr const char* TIME_ZONE = "EET-2EEDT,M3.5.0/3,M10.5.0/4"; // Europe/Athens
  constexpr uint8_t CONFIG_VERSION_DEVICE = 3;

// Device specific
  constexpr uint32_t SENSOR_INTERVAL = 30000;
  constexpr uint8_t RELAY_PIN = 14;
  constexpr uint8_t SENSOR_PIN = 12;
  constexpr uint8_t ONE_WIRE_PIN = 13;
  constexpr uint32_t SLEEP_DURATION_S = 60; // Seconds — actual sleep time may be slightly longer due to wakeup overhead

} // namespace DeviceDefaults

// example of custom Device event constants
constexpr EventType DEVICE_SENSOR_DATA_READY = EventType::APP_CUSTOM_EVENT_1;
constexpr EventType DEVICE_SENSOR_READ_ERROR = EventType::APP_CUSTOM_EVENT_2;
