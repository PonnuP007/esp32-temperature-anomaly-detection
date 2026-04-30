# ESP32 Temperature Anomaly Detection System

A real-time IoT system that detects temperature anomalies using an ESP32 microcontroller and Z-score statistical analysis.

## Architecture

DHT22 Sensor → ESP32 (C Firmware) → MQTT → Cloud Broker → Python Anomaly Detection
                                                                    ↓
                    ESP32 receives alert ←────── MQTT Alert ────────┘

## Features

- Custom DHT22 driver written in C (no external libraries)
- ESP32 firmware using FreeRTOS for real-time operation
- WiFi connectivity and MQTT-based cloud communication
- Z-score statistical anomaly detection in Python
- Live matplotlib dashboard showing temperature trends and anomalies
- Bidirectional MQTT alerts back to ESP32 when anomaly detected
- Simulated and tested in Wokwi embedded simulator

## Project Structure

```
esp32-temperature-anomaly-detection/
├── main/
│   ├── main.c          # Main application - WiFi, MQTT, sensor loop
│   ├── dht.c           # Custom DHT22 driver implementation
│   ├── dht.h           # DHT22 driver header
│   └── CMakeLists.txt
├── subscriber.py       # Python MQTT subscriber with anomaly detection
├── diagram.json        # Wokwi simulation wiring diagram
├── wokwi.toml          # Wokwi simulator configuration
├── CMakeLists.txt      # Project build configuration
└── README.md
```

## Tech Stack

- Firmware: C, ESP-IDF v6.0, FreeRTOS
- Hardware: ESP32, DHT22 temperature/humidity sensor
- Communication: WiFi, MQTT (HiveMQ broker)
- Anomaly Detection: Python, NumPy (Z-score method)
- Visualization: Matplotlib (live plot)
- Simulation: Wokwi embedded simulator

## How It Works

1. ESP32 reads temperature and humidity from DHT22 every 2 seconds
2. Data published as JSON to ponnu/temperature MQTT topic
3. Python subscriber applies Z-score anomaly detection
4. Readings deviating more than 2 standard deviations flagged as anomaly
5. Alert published back to ponnu/alerts MQTT topic
6. ESP32 receives and logs the alert in real time

## Setup

### Firmware
Install ESP-IDF v6.0, then:
```bash
idf.py build
```
Simulate with Wokwi VS Code extension

### Python
```bash
pip install paho-mqtt numpy matplotlib
python subscriber.py
```