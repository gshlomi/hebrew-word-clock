/*
 * Hebrew Word Clock — XIAO 7.5" ePaper (ESP32-C3)
 * ================================================
 * Displays current time in spoken Hebrew with full nikud.
 *
 * Hardware:
 *   Board   : Seeed XIAO ESP32-C3
 *   Display : 7.5" ePaper 800×480 (GDEW075T7 / UC8159 driver)
 *
 * Libraries (install via Arduino Library Manager):
 *   - GxEPD2          by ZinggJM   (≥ 1.5.8)
 *   - OpenFontRender  by takkaO    (≥ 1.3.0)  — TrueType on ESP32
 *   - NTPClient       by Fabrice   (≥ 3.2.1)
 *
 * Font:
 *   Download NotoSerifHebrew-Light.ttf from Google Fonts.
 *   Upload it to SPIFFS with:
 *     Arduino IDE → Tools → ESP32 Sketch Data Upload
 *     (place the .ttf file inside a "data/" folder next to the .ino)
 *
 * Pin wiring (XIAO ESP32-C3 → ePaper):
 *   D10 (GPIO10) → CS
 *   D8  (GPIO8)  → DC
 *   D9  (GPIO9)  → RST
 *   D7  (GPIO7)  → BUSY
 *   D4  (SDA/MOSI, GPIO4) → DIN
 *   D5  (SCL/CLK, GPIO5)  → CLK
 *   3.3 V → VCC
 *   GND  → GND
 *
 * WiFi credentials and timezone: edit the CONFIG section below.
 */

// ──────────────────────────────────────────────────────────────
//  CONFIG — edit here
// ──────────────────────────────────────────────────────────────
#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// UTC offset in seconds: Israel Standard Time = UTC+2 = 7200
// During DST (summer) Israel = UTC+3 = 10800
#define UTC_OFFSET_SEC  7200

// NTP server
#define NTP_SERVER "pool.ntp.org"

// How often to full-refresh the display (in minutes)
// ePaper partial refresh not used — full refresh every minute
#define REFRESH_INTERVAL_MIN 1

// Font file in SPIFFS
#define FONT_PATH "/NotoSerifHebrew-Light.ttf"

// ──────────────────────────────────────────────────────────────
//  INCLUDES
// ──────────────────────────────────────────────────────────────
#include <Arduino.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SPIFFS.h>
#include <SPI.h>

// GxEPD2 — choose the correct driver for the 7.5" 800×480 panel
// Common drivers for 7.5" 800×480: GxEPD2_750_T7 (GDEW075T7, UC8159)
#include <GxEPD2_BW.h>
#define GxEPD2_DISPLAY_CLASS GxEPD2_BW
#define GxEPD2_DRIVER_CLASS  GxEPD2_750_T7  // 800×480

// Pin definitions
#define EPD_CS   10
#define EPD_DC    8
#define EPD_RST   9
#define EPD_BUSY  7

// OpenFontRender for TrueType rendering
#include <OpenFontRender.h>

// ──────────────────────────────────────────────────────────────
//  GLOBALS
// ──────────────────────────────────────────────────────────────
GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, GxEPD2_DRIVER_CLASS::HEIGHT> display(
  GxEPD2_DRIVER_CLASS(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
);

WiFiUDP   ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, UTC_OFFSET_SEC, 60000);
OpenFontRender ofr;

// ──────────────────────────────────────────────────────────────
//  HEBREW TIME STRINGS (with full nikud, stored in PROGMEM)
// ──────────────────────────────────────────────────────────────

// Hours (feminine, 1–12)
const char H01[] PROGMEM = u8"אַחַת";
const char H02[] PROGMEM = u8"שְׁתַּיִם";
const char H03[] PROGMEM = u8"שָׁלוֹשׁ";
const char H04[] PROGMEM = u8"אַרְבַּע";
const char H05[] PROGMEM = u8"חָמֵשׁ";
const char H06[] PROGMEM = u8"שֵׁשׁ";
const char H07[] PROGMEM = u8"שֶׁבַע";
const char H08[] PROGMEM = u8"שְׁמוֹנֶה";
const char H09[] PROGMEM = u8"תֵּשַׁע";
const char H10[] PROGMEM = u8"עֶשֶׂר";
const char H11[] PROGMEM = u8"אַחַת עֶשְׂרֵה";
const char H12[] PROGMEM = u8"שְׁתֵּים עֶשְׂרֵה";
const char* const HOURS[13] PROGMEM = {
  nullptr, H01, H02, H03, H04, H05, H06,
  H07, H08, H09, H10, H11, H12
};

// Hours with לְ/לִ prefix (for "X to next hour")
const char T01[] PROGMEM = u8"לְאַחַת";
const char T02[] PROGMEM = u8"לִשְׁתַּיִם";
const char T03[] PROGMEM = u8"לְשָׁלוֹשׁ";
const char T04[] PROGMEM = u8"לְאַרְבַּע";
const char T05[] PROGMEM = u8"לְחָמֵשׁ";
const char T06[] PROGMEM = u8"לְשֵׁשׁ";
const char T07[] PROGMEM = u8"לְשֶׁבַע";
const char T08[] PROGMEM = u8"לִשְׁמוֹנֶה";
const char T09[] PROGMEM = u8"לְתֵשַׁע";
const char T10[] PROGMEM = u8"לְעֶשֶׂר";
const char T11[] PROGMEM = u8"לְאַחַת עֶשְׂרֵה";
const char T12[] PROGMEM = u8"לִשְׁתֵּים עֶשְׂרֵה";
const char* const HOURS_TO[13] PROGMEM = {
  nullptr, T01, T02, T03, T04, T05, T06,
  T07, T08, T09, T10, T11, T12
};

// Time-of-day suffixes
const char TOD_LAYLA[]  PROGMEM = u8"בַּלַּיְלָה";
const char TOD_LIFNOT[] PROGMEM = u8"לִפְנוֹת בּוֹקֶר";
const char TOD_BOKER[]  PROGMEM = u8"בַּבֹּקֶר";
const char TOD_AHAR[]   PROGMEM = u8"אַחַר הַצָּהֳרַיִם";
const char TOD_EREV[]   PROGMEM = u8"בָּעֶרֶב";

// Special words
const char CHATZOT[]    PROGMEM = u8"חֲצוֹת";
const char TZOHORAIM[]  PROGMEM = u8"צָהֳרַיִם";

// Minute fragments (forward counting)
const char M01[] PROGMEM = u8"וְדַקָּה";
const char M02[] PROGMEM = u8"וּשְׁתֵּי דַּקּוֹת";
const char M03[] PROGMEM = u8"וְשָׁלוֹשׁ דַּקּוֹת";
const char M04[] PROGMEM = u8"וְאַרְבַּע דַּקּוֹת";
const char M05[] PROGMEM = u8"וַחֲמִשָּׁה";
const char M06[] PROGMEM = u8"וְשֵׁשׁ דַּקּוֹת";
const char M07[] PROGMEM = u8"וְשֶׁבַע דַּקּוֹת";
const char M08[] PROGMEM = u8"וּשְׁמוֹנֶה דַּקּוֹת";
const char M09[] PROGMEM = u8"וְתֵשַׁע דַּקּוֹת";
const char M10[] PROGMEM = u8"וַעֲשָׂרָה";
const char M11[] PROGMEM = u8"וְאַחַת עֶשְׂרֵה דַּקּוֹת";
const char M12[] PROGMEM = u8"וּשְׁתֵּים עֶשְׂרֵה דַּקּוֹת";
const char M13[] PROGMEM = u8"וּשְׁלוֹשׁ עֶשְׂרֵה דַּקּוֹת";
const char M14[] PROGMEM = u8"וְאַרְבַּע עֶשְׂרֵה דַּקּוֹת";
const char M15[] PROGMEM = u8"וָרֶבַע";
const char M16[] PROGMEM = u8"וְשֵׁשׁ עֶשְׂרֵה דַּקּוֹת";
const char M17[] PROGMEM = u8"וּשְׁבַע עֶשְׂרֵה דַּקּוֹת";
const char M18[] PROGMEM = u8"וּשְׁמוֹנֶה עֶשְׂרֵה דַּקּוֹת";
const char M19[] PROGMEM = u8"וְתִשְׁעָה עֶשְׂרֵה דַּקּוֹת";
const char M20[] PROGMEM = u8"וְעֶשְׂרִים";
const char M21[] PROGMEM = u8"וְעֶשְׂרִים וְאַחַת";
const char M22[] PROGMEM = u8"וְעֶשְׂרִים וּשְׁתַּיִם";
const char M23[] PROGMEM = u8"וְעֶשְׂרִים וְשָׁלוֹשׁ";
const char M24[] PROGMEM = u8"וְעֶשְׂרִים וְאַרְבַּע";
const char M25[] PROGMEM = u8"וְעֶשְׂרִים וַחֲמִשָּׁה";
const char M26[] PROGMEM = u8"וְעֶשְׂרִים וָשֵׁשׁ";
const char M27[] PROGMEM = u8"וְעֶשְׂרִים וְשֶׁבַע";
const char M28[] PROGMEM = u8"וְעֶשְׂרִים וּשְׁמוֹנֶה";
const char M29[] PROGMEM = u8"וְעֶשְׂרִים וְתֵשַׁע";
const char M30[] PROGMEM = u8"וָחֵצִי";
const char M31[] PROGMEM = u8"שְׁלוֹשִׁים וְאַחַת";
const char M32[] PROGMEM = u8"שְׁלוֹשִׁים וּשְׁתַּיִם";
const char M33[] PROGMEM = u8"שְׁלוֹשִׁים וְשָׁלוֹשׁ";
const char M34[] PROGMEM = u8"שְׁלוֹשִׁים וְאַרְבַּע";
const char M36[] PROGMEM = u8"שְׁלוֹשִׁים וָשֵׁשׁ";
const char M37[] PROGMEM = u8"שְׁלוֹשִׁים וְשֶׁבַע";
const char M38[] PROGMEM = u8"שְׁלוֹשִׁים וּשְׁמוֹנֶה";
const char M39[] PROGMEM = u8"שְׁלוֹשִׁים וְתֵשַׁע";
const char M41[] PROGMEM = u8"אַרְבָּעִים וְאַחַת";
const char M42[] PROGMEM = u8"אַרְבָּעִים וּשְׁתַּיִם";
const char M43[] PROGMEM = u8"אַרְבָּעִים וְשָׁלוֹשׁ";
const char M44[] PROGMEM = u8"אַרְבָּעִים וְאַרְבַּע";
const char M46[] PROGMEM = u8"אַרְבָּעִים וָשֵׁשׁ";
const char M47[] PROGMEM = u8"אַרְבָּעִים וְשֶׁבַע";
const char M48[] PROGMEM = u8"אַרְבָּעִים וּשְׁמוֹנֶה";
const char M49[] PROGMEM = u8"אַרְבָּעִים וְתֵשַׁע";
const char M51[] PROGMEM = u8"חֲמִשִּׁים וְאַחַת";
const char M52[] PROGMEM = u8"חֲמִשִּׁים וּשְׁתַּיִם";
const char M53[] PROGMEM = u8"חֲמִשִּׁים וְשָׁלוֹשׁ";
const char M54[] PROGMEM = u8"חֲמִשִּׁים וְאַרְבַּע";
const char M56[] PROGMEM = u8"חֲמִשִּׁים וָשֵׁשׁ";
const char M57[] PROGMEM = u8"חֲמִשִּׁים וְשֶׁבַע";
const char M58[] PROGMEM = u8"חֲמִשִּׁים וּשְׁמוֹנֶה";

// "To" minute fragments
const char TO35[] PROGMEM = u8"עֶשְׂרִים וַחֲמִשָּׁה";
const char TO40[] PROGMEM = u8"עֶשְׂרִים";
const char TO45[] PROGMEM = u8"רֶבַע";
const char TO50[] PROGMEM = u8"עֲשָׂרָה";
const char TO55[] PROGMEM = u8"חֲמִשָּׁה";
const char TO59[] PROGMEM = u8"דַּקָּה";

// ──────────────────────────────────────────────────────────────
//  buildTimeString()
//  Returns the full Hebrew time string for given hour (0-23)
//  and minute (0-59) into buf (must be >= 120 bytes).
// ──────────────────────────────────────────────────────────────
void buildTimeString(int h24, int m, char* buf, size_t bufLen) {
  buf[0] = '\0';

  // ── Special cases ──
  if (h24 == 0 && m == 0) {
    strncpy_P(buf, CHATZOT, bufLen);
    return;
  }
  if (h24 == 12 && m == 0) {
    strncpy_P(buf, TZOHORAIM, bufLen);
    return;
  }

  // Time-of-day suffix
  char tod[40];
  int total = h24 * 60 + m;
  if      (total < 5*60)               strncpy_P(tod, TOD_LAYLA,  sizeof(tod));
  else if (h24 == 5)                   strncpy_P(tod, TOD_LIFNOT, sizeof(tod));
  else if (h24 < 12)                   strncpy_P(tod, TOD_BOKER,  sizeof(tod));
  else if (h24 == 12 && m > 0)        strncpy_P(tod, TOD_AHAR,   sizeof(tod));
  else if (total < 17*60)              strncpy_P(tod, TOD_AHAR,   sizeof(tod));
  else if (h24 < 21)                   strncpy_P(tod, TOD_EREV,   sizeof(tod));
  else                                 strncpy_P(tod, TOD_LAYLA,  sizeof(tod));

  int h12 = h24 % 12;
  if (h12 == 0) h12 = 12;

  // ── "To" form for minutes 35/40/45/50/55/59 ──
  const char* toFrag = nullptr;
  if      (m == 35) toFrag = TO35;
  else if (m == 40) toFrag = TO40;
  else if (m == 45) toFrag = TO45;
  else if (m == 50) toFrag = TO50;
  else if (m == 55) toFrag = TO55;
  else if (m == 59) toFrag = TO59;

  if (toFrag) {
    int next = (h12 % 12) + 1;
    char toMin[50], toHour[50];
    strncpy_P(toMin,  toFrag,          sizeof(toMin));
    strncpy_P(toHour, (char*)pgm_read_ptr(&HOURS_TO[next]), sizeof(toHour));
    snprintf(buf, bufLen, "%s %s %s", toMin, toHour, tod);
    return;
  }

  // ── Forward counting ──
  char hourStr[40];
  strncpy_P(hourStr, (char*)pgm_read_ptr(&HOURS[h12]), sizeof(hourStr));

  if (m == 0) {
    snprintf(buf, bufLen, "%s %s", hourStr, tod);
    return;
  }

  // Get minute fragment
  const char* mFrag = nullptr;
  switch (m) {
    case  1: mFrag = M01; break;  case  2: mFrag = M02; break;
    case  3: mFrag = M03; break;  case  4: mFrag = M04; break;
    case  5: mFrag = M05; break;  case  6: mFrag = M06; break;
    case  7: mFrag = M07; break;  case  8: mFrag = M08; break;
    case  9: mFrag = M09; break;  case 10: mFrag = M10; break;
    case 11: mFrag = M11; break;  case 12: mFrag = M12; break;
    case 13: mFrag = M13; break;  case 14: mFrag = M14; break;
    case 15: mFrag = M15; break;  case 16: mFrag = M16; break;
    case 17: mFrag = M17; break;  case 18: mFrag = M18; break;
    case 19: mFrag = M19; break;  case 20: mFrag = M20; break;
    case 21: mFrag = M21; break;  case 22: mFrag = M22; break;
    case 23: mFrag = M23; break;  case 24: mFrag = M24; break;
    case 25: mFrag = M25; break;  case 26: mFrag = M26; break;
    case 27: mFrag = M27; break;  case 28: mFrag = M28; break;
    case 29: mFrag = M29; break;  case 30: mFrag = M30; break;
    case 31: mFrag = M31; break;  case 32: mFrag = M32; break;
    case 33: mFrag = M33; break;  case 34: mFrag = M34; break;
    case 36: mFrag = M36; break;  case 37: mFrag = M37; break;
    case 38: mFrag = M38; break;  case 39: mFrag = M39; break;
    case 41: mFrag = M41; break;  case 42: mFrag = M42; break;
    case 43: mFrag = M43; break;  case 44: mFrag = M44; break;
    case 46: mFrag = M46; break;  case 47: mFrag = M47; break;
    case 48: mFrag = M48; break;  case 49: mFrag = M49; break;
    case 51: mFrag = M51; break;  case 52: mFrag = M52; break;
    case 53: mFrag = M53; break;  case 54: mFrag = M54; break;
    case 56: mFrag = M56; break;  case 57: mFrag = M57; break;
    case 58: mFrag = M58; break;
    default: break;
  }

  if (mFrag) {
    char minStr[60];
    strncpy_P(minStr, mFrag, sizeof(minStr));

    // Minutes 21–29, 31–34/36–39, 41–44/46–49, 51–54/56–58 are standalone
    // compound numbers → comma separator, strip leading ו + niqqud.
    bool needsComma = (m >= 21 && m <= 29) ||
                      (m >= 31 && m <= 39) ||
                      (m >= 41 && m <= 49) ||
                      (m >= 51 && m <= 58);

    if (needsComma) {
      // Skip the leading ו character (UTF-8: 0xD7 0x95) + optional niqqud byte
      char* p = minStr;
      if ((uint8_t)p[0] == 0xD7 && (uint8_t)p[1] == 0x95) {
        p += 2; // skip ו
        // skip one niqqud combining character if present (0xD6 0xBx..0xCx)
        if ((uint8_t)p[0] == 0xD6) p += 2;
      }
      snprintf(buf, bufLen, "%s, %s %s", hourStr, p, tod);
    } else {
      snprintf(buf, bufLen, "%s %s %s", hourStr, minStr, tod);
    }
  } else {
    snprintf(buf, bufLen, "%s %s", hourStr, tod);
  }
}

// ──────────────────────────────────────────────────────────────
//  drawClock()  — renders text centered on ePaper
// ──────────────────────────────────────────────────────────────
void drawClock(const char* text) {
  Serial.print("Drawing: ");
  Serial.println(text);

  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    // Configure font
    ofr.setFontSize(72);           // Large, adjust if text overflows
    ofr.setAlignment(Align::Center);
    ofr.setLayoutDirection(Layout::RightToLeft);

    // Display width = 800, height = 480
    // Center X: 400, Center Y: 240
    // Max text width: 50% of screen = 400px (25% margin each side)
    // If text wraps, let OpenFontRender handle it with a bounding box
    ofr.setDrawer(display);
    ofr.printf(400, 240, 400, 0, text);  // cx, cy, maxW, maxH

  } while (display.nextPage());
}

// ──────────────────────────────────────────────────────────────
//  SETUP
// ──────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Hebrew Word Clock ===");

  // Init SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("ERROR: SPIFFS mount failed");
    while (1) delay(1000);
  }
  if (!SPIFFS.exists(FONT_PATH)) {
    Serial.println("ERROR: Font file not found in SPIFFS: " FONT_PATH);
    Serial.println("Upload it via Tools → ESP32 Sketch Data Upload");
    while (1) delay(1000);
  }
  Serial.println("SPIFFS OK, font found");

  // Load font
  if (ofr.loadFont(FONT_PATH, SPIFFS) != OFR_OK) {
    Serial.println("ERROR: Failed to load font");
    while (1) delay(1000);
  }
  Serial.println("Font loaded");

  // Init display
  display.init(115200);
  display.setRotation(0);  // Landscape
  Serial.println("Display initialized");

  // Connect WiFi
  Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(500);
    Serial.print(".");
    tries++;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nERROR: WiFi failed — showing offline indicator");
    drawClock(u8"אין חיבור");
    return;
  }
  Serial.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());

  // Start NTP
  timeClient.begin();
  timeClient.update();
  Serial.printf("NTP time: %02d:%02d\n", timeClient.getHours(), timeClient.getMinutes());
}

// ──────────────────────────────────────────────────────────────
//  LOOP — refresh every minute
// ──────────────────────────────────────────────────────────────
void loop() {
  timeClient.update();

  int h = timeClient.getHours();
  int m = timeClient.getMinutes();

  char timeStr[160];
  buildTimeString(h, m, timeStr, sizeof(timeStr));

  drawClock(timeStr);

  // Sleep until the next minute boundary
  int secondsLeft = 60 - timeClient.getSeconds();
  Serial.printf("Next refresh in %d seconds\n", secondsLeft);
  delay((unsigned long)secondsLeft * 1000UL);
}
