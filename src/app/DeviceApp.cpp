#include "DeviceApp.h"

#include <cstring>

static void copyFixedString(char* dst, size_t dstSize, const char* src) {
    if (dstSize == 0) return;

    if (src == nullptr) {
        dst[0] = '\0';
        return;
    }

    strlcpy(dst, src, dstSize);
}

static FrameworkConfig createDeviceConfig() {
    FrameworkConfig cfg = {};
    copyFixedString(cfg.appName, sizeof(cfg.appName), DeviceDefaults::APP_NAME);
    copyFixedString(cfg.appVersion, sizeof(cfg.appVersion), DeviceDefaults::APP_VERSION);
    copyFixedString(cfg.appAuthor, sizeof(cfg.appAuthor), DeviceDefaults::APP_AUTHOR);
    copyFixedString(cfg.hostname, sizeof(cfg.hostname), DeviceDefaults::HOSTNAME);
    copyFixedString(cfg.apPassword, sizeof(cfg.apPassword), DeviceDefaults::WIFI_AP_PASSWORD);
    copyFixedString(cfg.mqttServer, sizeof(cfg.mqttServer), Defaults::MQTT_SERVER);
    copyFixedString(cfg.mqttUser, sizeof(cfg.mqttUser), Defaults::MQTT_USER);
    copyFixedString(cfg.mqttPassword, sizeof(cfg.mqttPassword), Defaults::MQTT_PASSWORD);
    copyFixedString(cfg.mqttClientId, sizeof(cfg.mqttClientId), DeviceDefaults::MQTT_CLIENT_ID);
    copyFixedString(cfg.baseTopic, sizeof(cfg.baseTopic), DeviceDefaults::BASE_TOPIC);
    copyFixedString(cfg.haTopic, sizeof(cfg.haTopic), Defaults::HA_TOPIC);
    copyFixedString(cfg.ntpServer, sizeof(cfg.ntpServer), DeviceDefaults::NTP_SERVER);
    copyFixedString(cfg.posixTimezone, sizeof(cfg.posixTimezone), DeviceDefaults::POSIX_TIME_ZONE);
    copyFixedString(cfg.otaUrl, sizeof(cfg.otaUrl), DeviceDefaults::OTA_URL);
    copyFixedString(cfg.otaKey, sizeof(cfg.otaKey), DeviceDefaults::OTA_KEY);
    cfg.configVersionBase   = Defaults::CONFIG_VERSION_BASE;
    cfg.configVersionDevice = DeviceDefaults::CONFIG_VERSION_DEVICE;
    return cfg;
}

// Use a function-local static to guarantee initialization order
const FrameworkConfig& deviceConfig() {
    static const FrameworkConfig cfg = createDeviceConfig();
    return cfg;
}

DeviceConfig::DeviceConfig() : configManager(eventBus, deviceConfig()) {}

DeviceApp::DeviceApp()
    : DeviceConfig()       // constructed first
      ,
      App(DeviceConfig::eventBus, DeviceConfig::configManager)      // safe — already constructed
      ,
      _relay(DeviceConfig::eventBus, App::_ha, DeviceDefaults::RELAY_PIN)
      // , _sensor(DeviceConfig::eventBus, App::_ha, DeviceDefaults::SENSOR_PIN)
      ,
      _sleepableSensor(DeviceConfig::eventBus, App::_ha, App::_sleep, DeviceDefaults::SENSOR_PIN),
      _ds18b20(DeviceConfig::eventBus, App::_ha, App::_sleep, DeviceDefaults::ONE_WIRE_PIN)

{}
void DeviceApp::begin() {
    addHandler(_relay);
    // addHandler(_sensor);
    addHandler(_sleepableSensor);
    addHandler(_ds18b20);
    App::begin();

  // when temperature read is complete (periodic or sleep-triggered), we can decide to go to sleep immediately
    _ds18b20.setReadCompleteCallback([this](bool success, size_t sensorCount) {
    // SleepRequest req(DeviceDefaults::SLEEP_DURATION_S);
    // _eventBus.publish(EventType::APP_SYSTEM_SLEEP_PREPARING, &req);
    });
}