# E-Ink Weather Station

![IMG_7173](https://github.com/user-attachments/assets/415caaca-9e34-4ed2-b476-34497108d246)

This project is an Arduino-based weather station that displays real-time weather data and a clock on a CrowPanel ESP32 4.2inch e-paper display. It fetches weather data from OpenWeatherMap and sensor readings via MQTT, updating the display with relevant icons and values.

**This project uses the CrowPanel ESP32 4.2inch E-paper HMI Display.**

## Features

- **E-paper display**: Uses GxEPD2 library for crisp, low-power visuals.
- **Weather icons**: Automatically fetches and displays weather icons from OpenWeatherMap.
- **MQTT integration**: Receives temperature, humidity, and pressure data from a sensor via MQTT.
- **Clock and date**: Synchronizes time using NTP and displays it on the screen.
- **Partial updates**: Efficiently redraws only changed portions of the display.

## Hardware Requirements

- CrowPanel ESP32 4.2inch E-paper HMI Display
- WiFi connectivity
- MQTT sensor publishing to a topic (e.g., `zigbee2mqtt/garden`)

## Software Requirements

- Arduino IDE or PlatformIO
- OpenWeatherMap API key

> **Tip:** If you want to set up the Arduino IDE for the CrowPanel ESP32 4.2inch E-paper HMI Display, you can use the official tutorial provided by the manufacturer:  
> [CrowPanel ESP32 E-Paper 4.2-inch Arduino Tutorial](https://www.elecrow.com/wiki/CrowPanel_ESP32_E-Paper_4.2-inch_Arduino_Tutorial.html)

## Setup

1. **Configure WiFi and MQTT**
   - Set your WiFi SSID and password:
     ```arduino
     const char* ssid = "your_wifi_ssid";
     const char* password = "your_wifi_password";
     ```
   - Set your MQTT broker address and topic:
     ```arduino
     const char* mqtt_server = "your_mqtt_broker";
     const char* mqtt_topic = "zigbee2mqtt/garden";
     ```

2. **Configure Weather API**
   - Get your OpenWeatherMap API key and set your latitude/longitude:
     ```arduino
     const char* weatherApiUrl = "https://api.openweathermap.org/data/2.5/weather?lat=XX.XX&lon=YY.YY&appid=YOUR_API_KEY";
     ```

3. **Upload and Run**
   - Compile and upload the code to your ESP32.
   - The display will show weather data, sensor readings, and the current time.

## How It Works

- **Weather Icon**: Every 5 minutes, the device fetches weather data from OpenWeatherMap, parses the JSON, and displays the corresponding icon.
- **Sensor Data**: When MQTT messages arrive, the device parses temperature, humidity, and pressure, then updates the display.
- **Clock**: The time is synchronized via NTP and updated every minute.
- **Display Updates**: Only changed areas of the display are refreshed for efficiency.


## Customization

- Change the MQTT topic or broker as needed.
- Add more weather icons to `pic.h` if desired.
- Adjust display layout by modifying drawing functions.
