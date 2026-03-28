# שעון מילים עברי — Arduino / ESP32-C3

קוד Arduino להפעלת השעון ישירות על XIAO 7.5" ePaper Panel.

---

## דרישות חומרה

| רכיב | מוצר |
|------|------|
| לוח | Seeed XIAO ESP32-C3 |
| מסך | XIAO 7.5" ePaper Panel (800×480, מובנה בלוח) |

---

## ספריות — התקנה ב-Arduino IDE

פתח את **Tools → Manage Libraries** והתקן:

| שם ספרייה | מחבר | גרסה מינימלית |
|-----------|------|---------------|
| `GxEPD2` | ZinggJM | ≥ 1.5.8 |
| `OpenFontRender` | takkaO | ≥ 1.3.0 |
| `NTPClient` | Fabrice Weinberg | ≥ 3.2.1 |

---

## גופן

1. הורד את **NotoSerifHebrew-Light.ttf** מ:
   https://fonts.google.com/noto/specimen/Noto+Serif+Hebrew

2. צור תיקייה `data/` בתוך תיקיית הסקיצה (ליד `hebrew_clock.ino`):
   ```
   hebrew_clock/
   ├── hebrew_clock.ino
   └── data/
       └── NotoSerifHebrew-Light.ttf
   ```

3. העלה לפלאש (SPIFFS) דרך:
   **Tools → ESP32 Sketch Data Upload**
   (דורש תוסף: https://github.com/lorol/arduino-esp32fs-plugin)

---

## הגדרות

ערוך את תחילת הקובץ:

```cpp
#define WIFI_SSID     "שם_הרשת_שלך"
#define WIFI_PASSWORD "סיסמה_שלך"
#define UTC_OFFSET_SEC  7200   // ישראל UTC+2 (קיץ: 10800)
```

---

## חיבור פינים

הפאנל של Seeed XIAO 7.5" מגיע עם קונקטור מוכן — אין צורך בחיווט ידני.
אם יש חיבור ידני:

| XIAO ESP32-C3 | ePaper |
|---------------|--------|
| D10 (GPIO10) | CS |
| D8 (GPIO8) | DC |
| D9 (GPIO9) | RST |
| D7 (GPIO7) | BUSY |
| D4 / MOSI | DIN |
| D5 / SCK | CLK |
| 3.3V | VCC |
| GND | GND |

---

## הגדרת לוח ב-Arduino IDE

- **Board:** XIAO ESP32-C3
  (דרך Boards Manager: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`)
- **Partition Scheme:** Default 4MB with SPIFFS
- **Upload Speed:** 921600

---

## סנן ה-ePaper הנכון

המסך 7.5" 800×480 משתמש ב-driver **UC8159 / GDEW075T7**.
בקוד מוגדר:
```cpp
#define GxEPD2_DRIVER_CLASS  GxEPD2_750_T7
```
אם המסך שלך שונה, בדוק את רשימת ה-drivers של GxEPD2:
https://github.com/ZinggJM/GxEPD2#supported-displays

---

## הערות

- **ניקוד עברי**: נשמר כ-UTF-8 literal בקוד. ה-IDE ו-OpenFontRender תומכים ב-Unicode מלא.
- **רענון**: המסך מתרענן כל דקה (בשינוי הדקה). ePaper אינו מתאים לרענון מהיר יותר.
- **RTL**: OpenFontRender מוגדר ל-`RightToLeft` כך שהטקסט מוצג נכון.
- **ללא WiFi**: אם החיבור נכשל, המסך יציג "אין חיבור".

---

## מבנה הקוד

```
setup()
 ├── init SPIFFS + load font
 ├── init ePaper display
 └── connect WiFi + start NTP

loop()
 ├── update NTP
 ├── buildTimeString(h, m) → Hebrew text
 ├── drawClock(text) → render on ePaper
 └── sleep until next minute
```
