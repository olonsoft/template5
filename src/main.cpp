#include <Arduino.h>

#include "app/DeviceApp.h"

DeviceApp app;

void setup() { app.begin(); }
void loop()  { app.loop();  }