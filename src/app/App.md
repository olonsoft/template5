**App**, the top‑level orchestrator of your entire firmware.

This is where everything comes together:

- StateMachine    
- ManageWiFi    
- ManageMQTT    
- ManageHa    
- ManageOta    
- Config    
- EventBus    
- Logger    

And App becomes the **single place** where you:

- Register Home Assistant entities    
- Register MQTT message handlers    
- React to state transitions    
- Publish sensor states    
- Implement device logic    
- Handle OTA events    
- Handle config changes    

This keeps your architecture clean, modular, and future‑proof.

Below is a complete, production‑ready App module.

# 🎯 **App Design Goals**

### ✔ Central coordinator

App owns all managers and wires them together.

### ✔ No business logic in managers

Managers only manage their domain.

### ✔ App registers HA entities

Sensors, switches, binary sensors, etc.

### ✔ App handles MQTT commands

Example: switch commands, config commands, custom commands.

### ✔ App reacts to StateMachine transitions

Example: when entering RUNNING → start publishing states.

### ✔ App publishes sensor updates

Example: temperature, uptime, RSSI.

### ✔ App handles OTA events

Example: show progress, stop publishing states during update.

### ✔ App loop is lightweight

No blocking, no delays.