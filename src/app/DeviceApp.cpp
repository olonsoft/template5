#include "DeviceApp.h"

const FrameworkConfig deviceConfig = {
    .appName = DeviceDefaults::APP_NAME,
                                       .appVersion = DeviceDefaults::APP_VERSION,
                                       .appAuthor = DeviceDefaults::APP_AUTHOR,
                                       .hostname = DeviceDefaults::HOSTNAME,
                                       .apPassword = DeviceDefaults::WIFI_AP_PASSWORD,
                                       .mqttServer = Defaults::MQTT_SERVER,
                                       .mqttUser = Defaults::MQTT_USER,
                                       .mqttPassword = Defaults::MQTT_PASSWORD,
                                       .mqttClientId = DeviceDefaults::MQTT_CLIENT_ID,
                                       .baseTopic = DeviceDefaults::BASE_TOPIC,
                                       .haTopic = Defaults::HA_TOPIC,
                                       .timeServer = DeviceDefaults::TIME_SERVER,
                                       .timeZone = DeviceDefaults::TIME_ZONE,
                                       .otaUrl = DeviceDefaults::OTA_URL,
                                       .configVersionBase = Defaults::CONFIG_VERSION_BASE,
                                       .configVersionDevice = DeviceDefaults::CONFIG_VERSION_DEVICE,
};

DeviceConfig::DeviceConfig()
    : configManager(eventBus, deviceConfig)
{}

DeviceApp::DeviceApp()
    : DeviceConfig()                                    // constructed first
    , App(DeviceConfig::eventBus,
          DeviceConfig::configManager)                  // safe — already constructed
    , _relay(DeviceConfig::eventBus, App::_ha,
             DeviceDefaults::RELAY_PIN)
    // , _sensor(DeviceConfig::eventBus, App::_ha,
    //           DeviceDefaults::SENSOR_PIN)
    , _sleepableSensor(DeviceConfig::eventBus, App::_ha,
                       App::_sleep, DeviceDefaults::SENSOR_PIN)
    , _tempSensors(DeviceConfig::eventBus, App::_ha,
                   DeviceDefaults::ONE_WIRE_PIN)

{}
void DeviceApp::begin() {
    addHandler(_relay);
    // addHandler(_sensor);
    addHandler(_sleepableSensor);
    addHandler(_tempSensors);
    App::begin();
}