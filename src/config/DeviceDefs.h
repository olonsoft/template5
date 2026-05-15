#pragma once
#include <Arduino.h>
#include <config/FrameworkDefs.h>
#include <core/EventBus.h>

namespace DeviceDefaults {
  // Device identity
constexpr char APP_NAME[CONFIG_APP_NAME_LEN]       = "Boiler temperature";
constexpr char APP_VERSION[CONFIG_APP_VERSION_LEN] = "1.1.0";
constexpr char APP_AUTHOR[CONFIG_APP_AUTHOR_LEN]   = "Dimitris";

  // Overrides
constexpr char HOSTNAME[CONFIG_HOSTNAME_LEN]              = "boiler-temp-$mac";
constexpr char WIFI_AP_PASSWORD[CONFIG_AP_PASSWORD_LEN]   = "12345678";
constexpr char MQTT_CLIENT_ID[CONFIG_MQTT_CLIENT_ID_LEN]  = "boiler-temp-$mac";
constexpr char BASE_TOPIC[CONFIG_BASE_TOPIC_LEN]          = "olon/home";
constexpr char OTA_URL[CONFIG_OTA_URL_LEN]                = "http://example.com/update.php";
constexpr char OTA_KEY[CONFIG_OTA_KEY_LEN]                = "gMqIpMJt4wjzsLpwVcNEwvsWygav1aJC";
constexpr char NTP_SERVER[CONFIG_NTP_SERVER_LEN]          = "time.ics.forth.gr";
constexpr char POSIX_TIME_ZONE[CONFIG_POSIX_TIMEZONE_LEN] = "EET-2EEDT,M3.5.0/3,M10.5.0/4";  // Europe/Athens
constexpr uint8_t CONFIG_VERSION_DEVICE                   = 3;

// Device specific
constexpr uint8_t RELAY_PIN  = 14;
constexpr uint8_t SENSOR_PIN = 12;

// DS18B20 specific
constexpr uint32_t SENSOR_INTERVAL  = 60000;
constexpr uint8_t ONE_WIRE_PIN      = 13;
constexpr uint32_t SLEEP_DURATION_S = 60;  // Seconds — actual sleep time may be slightly longer due to wakeup overhead

}  // namespace DeviceDefaults

// example of custom Device event constants
constexpr EventType DEVICE_SENSOR_DATA_READY = EventType::APP_CUSTOM_EVENT_1;
constexpr EventType DEVICE_SENSOR_READ_ERROR = EventType::APP_CUSTOM_EVENT_2;
