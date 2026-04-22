DS18B20 Temperature Module

### _Async, Non‑Blocking, Sleep‑Aware Sensor Handler for ESP8266/ESP32_

This module provides a **fully asynchronous**, **non‑blocking**, and **sleep‑aware** DS18B20 temperature sensor handler designed for low‑power ESP8266/ESP32 applications.

It integrates tightly with:

- **EventBus**
- **ManageHa** (Home Assistant auto‑discovery + JSON state publishing)
- **ManageSleep** (multi‑phase deep sleep controller)
- **AppHandler / ISleepable** framework

The design ensures:

- Zero blocking delays
- Single publish per cycle
- No wasted conversions
- Clean deep‑sleep transitions
- Robust behavior even when sleep is requested mid‑conversion

---

## Features

### ✔ Fully asynchronous DS18B20 operation

Uses `setWaitForConversion(false)` and a broadcast conversion request.

### ✔ Non‑blocking state machine

The main loop remains responsive at all times.

### ✔ Sleep‑aware

- If sleep is requested during a conversion → waits for it to finish
- If last reading is fresh → skips final read
- If no fresh data → performs one final read
- No periodic reads during shutdown

### ✔ Home Assistant auto‑discovery

Each sensor is exposed as a separate HA entity.

### ✔ Multi‑sensor support

Up to 10 DS18B20 sensors on a single OneWire bus.

### ✔ Optional “sleep after first read” mode

Useful for battery‑powered devices.

---

## Architecture Overview

The module uses a **three‑state** internal state machine:

```
┌──────────────┐
│ IDLE         │
└──────┬───────┘
│ periodic interval elapsed ▼
┌──────────────┐
│ PERIODIC_WAIT │ ← async conversion in progress
└──────┬────────┘
│ conversion time elapsed ▼
┌──────────────┐
│ finish()
│ → publish → back to IDLE
└──────────────┘
```

When sleep is requested:

```
Sleep requested
  │
  ├── If PERIODIC_WAIT → wait for finish()
  ├── If IDLE + fresh data → skip final read
  └── If IDLE + stale data → start SLEEP_WAIT
```

Sleep read state:

```
┌──────────────┐
│ SLEEP_WAIT   │ ← async conversion in progress
└──────┬───────┘
       │ conversion time elapsed ▼
┌──────────────┐
│ finish()
│ → publish → IDLE → sleep manager continues
└──────────────┘
```

---

## Sleep Behavior

### When sleep is requested:

| Condition                  | Behavior                      |
| -------------------------- | ----------------------------- |
| Conversion in progress     | Wait for it to finish         |
| Last read < 2 seconds old  | Skip final read               |
| No recent data             | Perform one async sleep read  |
| No sensors / MQTT offline  | Skip read                     |

### `isSleepDone()`

Returns `true` only when the module is in `IDLE` state.
This ensures the sleep manager waits for DS18B20 only when necessary.

---

## Public API (Signatures Only)

```cpp

DS18B20Handler(EventBus& bus,
               ManageHa& ha,
               ManageSleep& sleep,
               uint8_t pin,
               uint32_t interval = DeviceDefaults::SENSOR_INTERVAL);

void begin() override;
void loop() override;

void onSleep() override;
bool isSleepDone() const override;

void setSleepAfterRead(uint64_t durationUs = 0);
```

## Home Assistant Integration

Each DS18B20 sensor is exposed as:

```
sensor.temp_<shortId>
```

Example:

```
sensor.temp_5c9105
sensor.temp_ae8a04
```

The module publishes a single JSON payload:

```json
{
  "5c9105abcd1234ef": 22.06,
  "ae8a04abcd5678ff": 22.13
}
```

HA entities extract their values via `value_template`.

## Timing Diagram

```
Time →
┌──────────────────────────────────────────────────────────────┐
│ Periodic interval reached                                    │
│ requestTemperatures() (async)                                │
│                ┌───────────────────────────────┐             │
│                │ conversion in progress        │             │
│                └───────────────────────────────┘             │
│ finishConversion() → publish → IDLE                          │
│                                                              │
│ Sleep requested                                              │
│   ├─ If fresh → skip                                         │
│   ├─ If converting → wait                                    │
│   └─ Else → start sleep read                                 │
└──────────────────────────────────────────────────────────────┘
```

## Freshness Logic

A reading is considered **fresh** if:

```cpp
(now - _lastRead) < 2000 ms
```

This prevents unnecessary sleep reads and saves ~750 ms per cycle.

## Error Handling

Invalid readings are skipped if:

- `DEVICE_DISCONNECTED_C`
- `85.0°C` (power‑on default)
- `NaN`

A recoverable error event is published via EventBus.

## Example Log (Ideal Behavior)

```
Starting async broadcast conversion (periodic)
22.06°C
22.13°C
Read complete — triggering sleep
Sleep — last reading is fresh, skipping final read
All phases complete — entering deep sleep
```
