#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <time.h>

#include "clock.h"
#include "secrets.h"

const char *NTP_SERVER_1 = "pool.ntp.org";
const char *NTP_SERVER_2 = "de.pool.ntp.org";
// Central European Time with automatic DST (last Sunday of March/October)
const char *TZ_INFO = "CET-1CEST,M3.5.0,M10.5.0/3";
const unsigned long NTP_SYNC_INTERVAL_MS = 3600000UL;  // once per hour

ESP8266WebServer server(80);

bool ledState = false;         // false = LED on (active low)
unsigned long lastBlink = 0;

bool timeSynced = false;
unsigned long lastNtpLog = 0;

// Word clock frame: frame[0..2] are the R, G and B planes,
// each [row][column] holding the color value of that letter cell.
char frame[3][11][11];
int lastMinute = -1;

String getLocalTimeString() {
  time_t now = time(nullptr);
  struct tm tmNow;
  localtime_r(&now, &tmNow);
  char buf[32];
  strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M:%S", &tmNow);
  return String(buf);
}

// Rebuilds the frame when the minute changes.
void updateFrame() {
  time_t now = time(nullptr);
  if (now <= 1000000000UL) {
    return;  // NTP not synced yet
  }
  struct tm tmNow;
  localtime_r(&now, &tmNow);
  if (tmNow.tm_min != lastMinute) {
    lastMinute = tmNow.tm_min;
    WordClock::buildFrame((uint8_t)tmNow.tm_hour, (uint8_t)tmNow.tm_min, frame);
  }
}

// Fills the frame with all letters dim gray (shown before the first NTP sync).
void initFrameGray() {
  for (uint8_t r = 0; r < WordClock::ROWS; ++r) {
    for (uint8_t c = 0; c < WordClock::COLS; ++c) {
      const char *color = WordClock::CELL[r][c] ? WordClock::COLOR_GRAY : WordClock::COLOR_OFF;
      frame[0][r][c] = color[0];
      frame[1][r][c] = color[1];
      frame[2][r][c] = color[2];
    }
  }
}

void handleRoot() {
  String html = F(
      "<!DOCTYPE html>"
      "<html><head><meta charset='utf-8'>"
      "<meta http-equiv='refresh' content='60'>"
      "<title>Word Clock</title>"
      "<style>body{background:#000;font-family:monospace;text-align:center}"
      "table{border-collapse:collapse;margin:24px auto;font-size:28px}"
      "td{width:1.2em;height:1.2em;text-align:center}</style></head><body>"
      "<table>");
  for (uint8_t r = 0; r < WordClock::ROWS; ++r) {
    html += F("<tr>");
    for (uint8_t c = 0; c < WordClock::COLS; ++c) {
      const char *letter = WordClock::CELL[r][c];
      if (letter) {
        char buf[80];
        snprintf(buf, sizeof(buf),
                 "<td style='color:rgb(%d,%d,%d)'>%s</td>",
                 (int)(unsigned char)frame[0][r][c],
                 (int)(unsigned char)frame[1][r][c],
                 (int)(unsigned char)frame[2][r][c],
                 letter);
        html += buf;
      } else {
        html += F("<td></td>");
      }
    }
    html += F("</tr>");
  }
  html += F("</table><p>");
  html += timeSynced ? getLocalTimeString() : String(F("waiting for NTP sync..."));
  html += F("</p></body></html>");
  server.send(200, F("text/html"), html);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println(F("Starting word clock server..."));

  initFrameGray();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, LOW);  // LED on (active low) while connecting
    delay(250);
    Serial.print(".");
    if (millis() - start > 30000) {
      Serial.println();
      Serial.println(F("WiFi connect timeout, rebooting..."));
      delay(100);
      ESP.restart();
    }
  }

  Serial.println();
  Serial.print("Connected, IP via DHCP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("Signal (RSSI): ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");

  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println(F("HTTP server started on port 80"));

  // SNTP: syncs on start, then re-syncs every hour
  // TZ-string overload; the configTime(0, 0, ...) int overload would
  // overwrite TZ and give UTC only
  configTime(TZ_INFO, NTP_SERVER_1, NTP_SERVER_2);
  Serial.print("NTP sync started (");
  Serial.print(NTP_SERVER_1);
  Serial.print(", ");
  Serial.print(NTP_SERVER_2);
  Serial.println("), re-sync interval 1 h");
}

void loop() {
  server.handleClient();
  updateFrame();

  if (!timeSynced && time(nullptr) > 1000000000UL) {
    timeSynced = true;
    lastNtpLog = millis();
    Serial.print("NTP time synced: ");
    Serial.println(getLocalTimeString());
  } else if (timeSynced && millis() - lastNtpLog >= NTP_SYNC_INTERVAL_MS) {
    lastNtpLog = millis();
    Serial.print("NTP re-sync (hourly), current time: ");
    Serial.println(getLocalTimeString());
  }

  // non-blocking 1 Hz blink, replaces the original delay(1000) loop
  if (millis() - lastBlink >= 500) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);  // active low
  }
}
