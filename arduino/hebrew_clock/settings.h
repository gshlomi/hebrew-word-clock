#pragma once
#include <Preferences.h>
#include <atomic>
#include "timezones.h"
namespace Settings {
struct Values {
  uint32_t version;
  char zone[64];
  char rule[128];
  char ip[16];
  uint16_t port;
  uint8_t enabled;
};
Values values{};
SemaphoreHandle_t timeLock;
std::atomic<unsigned> revision{0};
const char* ruleFor(const String& zone) {
  if (zone == "Firmware default") return CLOCK_TIMEZONE;
  for (const auto& entry : TIMEZONES) if (zone == entry.name) return entry.rule;
  return nullptr;
}
bool valid(const Values& v) {
  if (v.version != 1 || !memchr(v.zone, 0, sizeof(v.zone)) ||
      !memchr(v.rule, 0, sizeof(v.rule)) || !memchr(v.ip, 0, sizeof(v.ip)) ||
      !v.port || v.enabled > 1) return false;
  IPAddress ip;
  return ruleFor(v.zone) && v.rule[0] && (!v.ip[0] || ip.fromString(v.ip)) && (!v.enabled || v.ip[0]);
}
void apply() {
  xSemaphoreTake(timeLock, portMAX_DELAY);
  setenv("TZ", values.rule, 1); tzset();
  ++revision;
  xSemaphoreGive(timeLock);
}
void local(time_t now, tm* result) {
  xSemaphoreTake(timeLock, portMAX_DELAY);
  localtime_r(&now, result);
  xSemaphoreGive(timeLock);
}
void begin() {
  timeLock = xSemaphoreCreateMutex();
  configASSERT(timeLock);
  values.version = 1;
  strlcpy(values.zone, "Firmware default", sizeof(values.zone));
  strlcpy(values.rule, CLOCK_TIMEZONE, sizeof(values.rule));
  strlcpy(values.ip, CLOCK_SYSLOG_IP, sizeof(values.ip));
  values.port = CLOCK_SYSLOG_PORT;
  IPAddress ip; values.enabled = values.ip[0] && ip.fromString(values.ip);
  Preferences prefs;
  if (prefs.begin("clock-settings", true)) {
    Values saved{};
    if (prefs.getBytesLength("values") == sizeof(saved) &&
        prefs.getBytes("values", &saved, sizeof(saved)) == sizeof(saved) && valid(saved)) values = saved;
    prefs.end();
  }
  apply();
}
bool save(const Values& next) {
  if (!valid(next)) return false;
  Preferences prefs;
  if (!prefs.begin("clock-settings", false)) return false;
  bool ok = prefs.putBytes("values", &next, sizeof(next)) == sizeof(next);
  prefs.end();
  if (ok) { values = next; apply(); }
  return ok;
}
}
