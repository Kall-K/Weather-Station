# Arduino Sensor Station

Firmware for an Arduino Uno sensor station that measures temperature, humidity, and soil moisture. Readings appear on a 16x2 LCD and are sent to a local HTTP server through an ESP8266 Wi-Fi module.

## Features

- Reads temperature and humidity from a DHT22.
- Reads an analog soil-moisture sensor.
- Shows live readings and connection status on a 16x2 LCD.
- Connects an ESP8266 to Wi-Fi with AT commands.
- Posts each sensor's readings as JSON to `/sensorValues`.

## Hardware and Wiring

| Component | Arduino Uno connection |
| --- | --- |
| DHT22 data | D6 |
| Soil-moisture analog output | A4 |
| ESP8266 serial | D7 (Arduino RX), D8 (Arduino TX) |
| LCD | RS D12, E D11, D4 D5, D5 D4, D6 D3, D7 D2 |

The LCD uses the `LiquidCrystal lcd(12, 11, 5, 4, 3, 2)` pin mapping. The ESP8266 and the USB serial monitor both run at 9600 baud.

## Project Layout

```text
include/config.h       Server settings, station ID, and sensor definitions
include/secret.h       Local Wi-Fi credentials (ignored by Git)
src/main.cpp           Firmware entry point
platformio.ini         Arduino Uno and PlatformIO configuration
```

## Configuration

`include/config.h` defines the station ID and sensor names used in outgoing payloads. Local Wi-Fi credentials and the server IP address and port are stored in `include/secret.h`, which is excluded. `include/secret.example.h` provides the expected structure without exposing local connection details.

## Runtime Behavior

At startup, the firmware checks that the ESP8266 responds, connects to Wi-Fi, then verifies that the configured server accepts a TCP connection. Connection progress and failures are displayed on the LCD.

Every 10 seconds, the station reads both sensors, refreshes the LCD, and sends readings when the server connection test was successful. The DHT22 values are sent first, followed one second later by the soil-moisture reading.

## Server API

The ESP8266 sends HTTP `POST` requests to:

```text
http://<SERVER_IP>:<SERVER_PORT>/sensorValues
```

Example DHT22 payload:

```json
{
  "stationId": 1,
  "sensorName": "DHT22",
  "readings": {
    "temperature": 24.5,
    "humidity": 48.0
  }
}
```

The soil sensor uses `sensorName` set to `SOIL_MOISTURE` and sends a `soil_moisture` field. Its value is the raw Arduino analog-to-digital converter reading rather than a percentage.

## ESP8266 Requirements

The ESP8266 must run AT firmware, communicate at 9600 baud, and be able to join the configured Wi-Fi network.
