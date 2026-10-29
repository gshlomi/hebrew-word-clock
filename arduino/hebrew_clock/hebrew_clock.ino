/* Hebrew Word Clock: Seeed XIAO 7.5-inch ePaper Panel (ESP32-C3).
 * Official Seeed_GFX preset 502: UC8179, 800x480, XIAO ESP32-C3.
 * See ../README.md for reproducible build and flashing instructions.
 */
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "driver.h"
#include <TFT_eSPI.h>
#include "time_words.h"
#include "word_bitmaps.h"
#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif

// Israel: last Friday in March at 02:00; last Sunday in October at 02:00.
// POSIX week-4 Thursday + 26 hours expresses the last Friday in March.
constexpr const char* ISRAEL_TZ = "IST-2IDT,M3.4.4/26,M10.5.0";
EPaper display;
int64_t lastMinute = -1;
uint32_t lastRetry = 0;
bool waitingForTime = false;
bool showedOffline = false;

const WordBitmap* findWord(const char* text) {
  for (size_t i=0; i<WORD_COUNT; ++i) {
    if (strcmp(text, WORDS[i].text) == 0) return &WORDS[i];
  }
  return nullptr;
}

void drawClock(const char* text) {
  char copy[160];
  snprintf(copy, sizeof(copy), "%s", text);
  const WordBitmap* words[24];
  int widths[8] = {}, starts[8] = {}, ends[8] = {};
  int count = 0, line = 0;
  char* state = nullptr;
  for (char* token = strtok_r(copy, " ", &state); token; token = strtok_r(nullptr, " ", &state)) {
    const WordBitmap* word = findWord(token);
    if (!word || count >= 24) { Serial.println("ERROR: regenerate word assets"); return; }
    int added = word->width + (widths[line] ? WORD_GAP : 0);
    if (widths[line] && widths[line] + added > TEXT_WIDTH) {
      ends[line] = count;
      if (++line >= 8) return;
      starts[line] = count;
      added = word->width;
    }
    widths[line] += added;
    words[count++] = word;
  }
  ends[line] = count;
  int totalHeight = (line+1)*WORD_HEIGHT + line*LINE_GAP;
  if (totalHeight > 400) { Serial.println("ERROR: text exceeds panel"); return; }
  display.fillScreen(TFT_WHITE);
  int y = (480-totalHeight)/2;
  for (int row=0; row<=line; ++row) {
    int x = (800+widths[row])/2;
    for (int i=starts[row]; i<ends[row]; ++i) {
      x -= words[i]->width;
      display.drawBitmap(x, y, words[i]->bits, words[i]->width, WORD_HEIGHT, TFT_BLACK);
      x -= WORD_GAP;
    }
    y += WORD_HEIGHT + LINE_GAP;
  }
  display.update(); // Full refresh; Seeed driver sleeps the panel afterward.
}

void setup() {
  Serial.begin(115200);
  display.begin();
  display.setRotation(0); // Native 800 x 480 landscape coordinates.
  if (!display.getPointer() || display.width()!=800 || display.height()!=480) {
    Serial.println("ERROR: display buffer or geometry");
    while (true) delay(1000);
  }
  drawClock("ממתין לשעה");
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  configTzTime(ISRAEL_TZ, "pool.ntp.org", "time.google.com");
  lastRetry = millis();
}

void loop() {
  const bool online = WiFi.status() == WL_CONNECTED;
  if (!online && millis()-lastRetry >= 30000UL) {
    WiFi.reconnect();
    lastRetry = millis();
  }
  time_t now = time(nullptr);
  // Never display uptime as a real clock. SNTP continues retrying in background.
  if (now < 1704067200) {
    if (!online && !showedOffline && millis()-lastRetry >= 15000UL) {
      drawClock("אין חיבור"); showedOffline = true; waitingForTime = false;
    } else if (online && !waitingForTime) {
      drawClock("ממתין לשעה"); waitingForTime = true; showedOffline = false;
    }
    delay(200);
    return;
  }
  // After first sync, ESP32 system time continues during temporary WiFi outages.
  if (now/60 != lastMinute) {
    tm local;
    localtime_r(&now, &local);
    char text[160];
    buildTimeString(local.tm_hour, local.tm_min, text, sizeof(text));
    drawClock(text);
    lastMinute = now/60;
    Serial.printf("%02d:%02d heap=%u\n", local.tm_hour, local.tm_min, ESP.getFreeHeap());
  }
  delay(200);
}
