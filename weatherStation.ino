#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <GxEPD2_BW.h>
#include <time.h>
#include <HTTPClient.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include "pic.h"

#define PWR 7
#define BUSY 48
#define RES 47
#define DC 46
#define CS 45

const char* ssid = "";
const char* password = "";
const char* mqtt_server = "";
const int mqtt_port = 1883;
const char* mqtt_topic = "zigbee2mqtt/garden"; // set your own topic
String lastUpdateTime = "";
unsigned long lastWeatherUpdate = 0;
String lastWeatherIcon = "";
const unsigned long weatherUpdateInterval = 300000; // 5min
const char* weatherApiUrl = "https://api.openweathermap.org/data/2.5/weather?lat=xxx&lon=xxx&appid=xxx"; // generate your own free API key and set lat/lon

WiFiClient espClient;
PubSubClient client(espClient);

GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT> epd(GxEPD2_420_GYE042A87(CS, DC, RES, BUSY));

void epdPower(bool on) {
  pinMode(PWR, OUTPUT);
  digitalWrite(PWR, on ? HIGH : LOW);
}

const unsigned char* mapIconToBitmap(const char* icon) {
  if (strcmp(icon, "01d") == 0) return gImage_01d;
  if (strcmp(icon, "01n") == 0) return gImage_01n;
  if (strcmp(icon, "02d") == 0) return gImage_02d;
  if (strcmp(icon, "02n") == 0) return gImage_02n;
  if (strcmp(icon, "03d") == 0) return gImage_03d;
  if (strcmp(icon, "03n") == 0) return gImage_03n;
  if (strcmp(icon, "04d") == 0 || strcmp(icon, "04n") == 0) return gImage_04d;
  if (strcmp(icon, "09d") == 0 || strcmp(icon, "09n") == 0 || strcmp(icon, "10d") == 0 || strcmp(icon, "10n") == 0) return gImage_10d;
  if (strcmp(icon, "11d") == 0 || strcmp(icon, "11n") == 0) return gImage_11d;
  if (strcmp(icon, "13d") == 0 || strcmp(icon, "13n") == 0) return gImage_13d;
  if (strcmp(icon, "50d") == 0 || strcmp(icon, "50n") == 0) return gImage_50d;
  return nullptr;
}

void fetchAndDrawWeatherIcon() {
  static unsigned long lastFetch = -weatherUpdateInterval;
  static String        lastIcon  = "";

  if (millis() - lastFetch < weatherUpdateInterval) return;
  lastFetch = millis();

  HTTPClient http;
  http.begin(weatherApiUrl);
  int code = http.GET();
  if (code != 200) { http.end(); return; }
  String payload = http.getString();
  http.end();

  StaticJsonDocument<1024> doc;
  if (deserializeJson(doc, payload) != DeserializationError::Ok) return;
  const char* icon = doc["weather"][0]["icon"];
  Serial.println(icon);

  if (lastIcon == icon) return;
  lastIcon = String(icon);

  const unsigned char* bm = mapIconToBitmap(icon);
  if (!bm) return;

  int iconW = 128, iconH = 128;
  int iconX = epd.width() - iconW, iconY = 0;
  epd.setPartialWindow(iconX, iconY, iconW, iconH);
  epd.firstPage();
  do {
    epd.fillRect(iconX, iconY, iconW, iconH, GxEPD_WHITE);
    epd.drawBitmap(iconX, iconY, bm + 6, iconW, iconH, GxEPD_BLACK);
  } while (epd.nextPage());
}

void drawLabeledValue(float value, int y, const char* unit, const unsigned char* icon, int iconW, int iconH) {
  int iconX = 10;
  int iconY = y - iconH + 8;
  int valueX = iconX + iconW + 10;

  epd.setFont(&FreeMonoBold24pt7b);
  String text = String(value, 1) + unit;

  int16_t tbx, tby;
  uint16_t tbw, tbh;
  epd.getTextBounds(text, 0, y, &tbx, &tby, &tbw, &tbh);

  int padding = 10;
  int clearY = y - tbh - padding;
  int clearH = tbh + 2 * padding;
  int clearX = iconX;
  int clearW = valueX + tbw + padding - clearX;

  epd.setPartialWindow(clearX, clearY, clearW, clearH);
  epd.firstPage();
  do {
    epd.fillRect(clearX, clearY, clearW, clearH, GxEPD_WHITE);
    epd.drawBitmap(iconX, iconY, icon + 6, iconW, iconH, GxEPD_BLACK);
    epd.setTextColor(GxEPD_BLACK);
    epd.setCursor(valueX, y);
    epd.print(text);
  } while (epd.nextPage());
}

void callback(char* topic, byte* message, unsigned int length) {
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message, length);
  if (error) return;

  float temperature = doc["temperature"];
  float humidity = doc["humidity"];
  float pressure = doc["pressure"];

  drawLabeledValue(temperature, 60, " C", gImage_temperature, 48, 49);
  drawLabeledValue(humidity,   133, " %", gImage_humidity,     48, 49);
  drawLabeledValue(pressure,   195, " hPa", gImage_pressure, 49, 32);

  time_t now;
  time(&now);
  struct tm* timeinfo = localtime(&now);
  char timeStr[6];
  strftime(timeStr, sizeof(timeStr), "%H:%M", timeinfo);
  lastUpdateTime = String(timeStr);

  drawLastUpdate();
}

void drawLastUpdate() { // this function draws the last update time on the bottom left corner of the display 
  epd.setFont(&FreeMonoBold9pt7b);
  epd.setTextColor(GxEPD_BLACK);

  int x = 5;
  int y = epd.height() - 5; 
  int w = 80;
  int h = 15;

  epd.setPartialWindow(x, y - h + 2, w, h);
  epd.firstPage();
  do {
    epd.fillRect(x, y - h + 2, w, h, GxEPD_WHITE);
    epd.setCursor(x, y);
    epd.print(lastUpdateTime);
  } while (epd.nextPage());
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("EInkClient")) {
      client.subscribe(mqtt_topic);
    } else {
      delay(5000);
    }
  }
}

void drawClock() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  char timeStr[6];
  strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);

  char dateStr[16];
  strftime(dateStr, sizeof(dateStr), "%d-%m-%Y", &timeinfo);

  int y = 277; 
  int paddingTop = 6; 

  int updateY = 240;
  int updateH = epd.height() - updateY;

  epd.setPartialWindow(80, updateY, epd.width() - 80, updateH);

  epd.firstPage();
  do {
    epd.fillRect(80, updateY, epd.width() - 80, updateH, GxEPD_WHITE);

    epd.setFont(&FreeMonoBold12pt7b);
    epd.setTextColor(GxEPD_BLACK);

    int16_t tbx, tby;
    uint16_t tbw, tbh;

    epd.getTextBounds(timeStr, 0, 0, &tbx, &tby, &tbw, &tbh);
    epd.setCursor((epd.width() - tbw) / 2, y);
    epd.print(timeStr);

    epd.getTextBounds(dateStr, 0, 0, &tbx, &tby, &tbw, &tbh);
    epd.setCursor((epd.width() - tbw) / 2, y + 20);
    epd.print(dateStr);

  } while (epd.nextPage());
}

void drawHorizontalLine() {
  int y = 234;
  epd.setPartialWindow(0, y, epd.width(), 1); 
  epd.firstPage();
  do {
    epd.drawLine(0, y, epd.width(), y, GxEPD_BLACK);
  } while (epd.nextPage());
}

void setup() {
  Serial.begin(115200);
  epdPower(true);
  epd.init(115200);
  epd.setRotation(0);
  drawHorizontalLine();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  fetchAndDrawWeatherIcon();

  configTime(3600, 3600, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
  tzset();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

int lastDisplayedMinute = -1;

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    if (timeinfo.tm_min != lastDisplayedMinute) {
      drawClock();
      lastDisplayedMinute = timeinfo.tm_min;
    }
  }

fetchAndDrawWeatherIcon();
}
