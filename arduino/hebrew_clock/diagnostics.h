#pragma once
#include <WebServer.h>
#include <WiFiUdp.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <stdarg.h>

// Only the diagnostic task owns HTTP/UDP. Display code only writes bounded records.
namespace Diagnostics {
struct Record { uint64_t ms; int severity; char text[160]; };
constexpr unsigned CAPACITY = 48;
Record records[CAPACITY];
unsigned head = 0, count = 0;
uint32_t dropped = 0;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
QueueHandle_t queue = nullptr;
uint64_t refreshStart = 0, refreshEnd = 0;
bool refreshing = false;
char phase[48] = "boot";
WebServer server(80);
WiFiUDP udp;
IPAddress syslogAddress;
bool syslogEnabled = false;
uint32_t syslogSent = 0, syslogFailed = 0;
uint64_t uptime() { return esp_timer_get_time() / 1000ULL; }

void log(int severity, const char* format, ...) {
  Record r{};
  r.ms = uptime(); r.severity = severity;
  va_list args; va_start(args, format);
  vsnprintf(r.text, sizeof(r.text), format, args); va_end(args);
  portENTER_CRITICAL(&mux);
  records[head] = r; head = (head + 1) % CAPACITY;
  if (count < CAPACITY) ++count;
  portEXIT_CRITICAL(&mux);
  if (queue && xQueueSend(queue, &r, 0) != pdTRUE) {
    portENTER_CRITICAL(&mux); ++dropped; portEXIT_CRITICAL(&mux);
  }
}
void setPhase(const char* value) {
  portENTER_CRITICAL(&mux);
  snprintf(phase, sizeof(phase), "%s", value);
  portEXIT_CRITICAL(&mux);
  log(6, "phase=%s", value);
}
void refresh(bool start) {
  portENTER_CRITICAL(&mux);
  refreshing = start;
  if (start) refreshStart = uptime(); else refreshEnd = uptime();
  portEXIT_CRITICAL(&mux);
  log(6, "display refresh %s", start ? "started" : "returned");
}
String logs() {
  String out; out.reserve(CAPACITY * 200);
  // Copy each record under a short lock; never perform I/O while locked.
  unsigned n, first;
  portENTER_CRITICAL(&mux); n = count; first = (head + CAPACITY - n) % CAPACITY; portEXIT_CRITICAL(&mux);
  for (unsigned i = 0; i < n; ++i) {
    Record r;
    portENTER_CRITICAL(&mux); r = records[(first + i) % CAPACITY]; portEXIT_CRITICAL(&mux);
    char prefix[48]; snprintf(prefix, sizeof(prefix), "[%llu ms] <%d> ", (unsigned long long)r.ms, r.severity);
    out += prefix; out += r.text; out += '\n';
  }
  return out;
}
String status() {
  char p[48]; uint64_t start, end; bool busy; uint32_t lost;
  portENTER_CRITICAL(&mux);
  memcpy(p, phase, sizeof(p)); start = refreshStart; end = refreshEnd; busy = refreshing; lost = dropped;
  portEXIT_CRITICAL(&mux);
  time_t now = time(nullptr); tm local{}; localtime_r(&now, &local);
  char date[64]; strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S %Z", &local);
  char buf[1200];
  snprintf(buf, sizeof(buf),
    "Hostname: %s\nUptime: %llu ms\nReset reason (esp_reset_reason): %d\nPhase: %s\n"
    "Time synced: %s\nLocal time: %s\nWiFi status: %d\nIP: %s\nRSSI: %d dBm\n"
    "Free heap: %u\nMinimum free heap: %u\nDisplay update in progress: %s\n"
    "Last update start: %llu ms\nLast update returned: %llu ms\n"
    "Syslog: %s:%u (%s)\nUDP packets sent locally: %u\nUDP send failures: %u\nLog queue drops: %u\n",
    CLOCK_HOSTNAME, (unsigned long long)uptime(), (int)esp_reset_reason(), p,
    now >= 1704067200 ? "yes" : "no", date, (int)WiFi.status(), WiFi.localIP().toString().c_str(), WiFi.RSSI(),
    ESP.getFreeHeap(), ESP.getMinFreeHeap(), busy ? "yes" : "no", (unsigned long long)start, (unsigned long long)end,
    CLOCK_SYSLOG_IP, (unsigned)CLOCK_SYSLOG_PORT, syslogEnabled ? "enabled" : "disabled", syslogSent, syslogFailed, lost);
  return String(buf);
}
const char PAGE[] PROGMEM = R"HTML(<!doctype html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>Hebrew clock diagnostics</title></head><body><h1>Hebrew clock diagnostics</h1><p>Read-only. Refreshes every 5 seconds. <a href="/logs">Download logs</a></p><p id="connection"></p><h2>Status</h2><pre id="status"></pre><h2>Recent logs</h2><pre id="logs"></pre><script>
async function update(){try{const results=await Promise.all(['/status','/logs'].map(async p=>{const r=await fetch(p,{cache:'no-store',signal:AbortSignal.timeout(4000)});if(!r.ok)throw Error(r.status);return r.text()}));document.getElementById('status').textContent=results[0];document.getElementById('logs').textContent=results[1];document.getElementById('connection').textContent='Updated '+new Date().toLocaleTimeString()}catch(e){document.getElementById('connection').textContent='Device unreachable — displayed data is stale.'}finally{setTimeout(update,5000)}}update();
</script></body></html>)HTML";
void task(void*) {
  syslogEnabled = strlen(CLOCK_SYSLOG_IP) && syslogAddress.fromString(CLOCK_SYSLOG_IP);
  if (strlen(CLOCK_SYSLOG_IP) && !syslogEnabled) log(3, "Invalid syslog IPv4 address; forwarding disabled");
  server.on("/", [] { server.send_P(200, "text/html; charset=utf-8", PAGE); });
  server.on("/status", [] { server.sendHeader("Cache-Control", "no-store"); server.send(200, "text/plain; charset=utf-8", status()); });
  server.on("/logs", [] { server.sendHeader("Cache-Control", "no-store"); server.sendHeader("Content-Disposition", "attachment; filename=clock.log"); server.send(200, "text/plain; charset=utf-8", logs()); });
  server.begin();
  int previous = -1; uint64_t retry = uptime(), heartbeat = 0; bool synced = false;
  for (;;) {
    int state = WiFi.status();
    if (state != previous) {
      log(6, "WiFi status=%d IP=%s RSSI=%d reset_reason=%d", state, WiFi.localIP().toString().c_str(), WiFi.RSSI(), (int)esp_reset_reason()); previous = state;
    }
    if (state != WL_CONNECTED && uptime() - retry >= 30000) { WiFi.reconnect(); retry = uptime(); }
    if (!synced && time(nullptr) >= 1704067200) { synced = true; log(6, "System time synchronized"); }
    if (uptime() - heartbeat >= 60000) {
      heartbeat = uptime(); log(6, "heartbeat heap=%u min_heap=%u WiFi=%d", ESP.getFreeHeap(), ESP.getMinFreeHeap(), state);
    }
    server.handleClient();
    Record r;
    // Bound each batch so logging cannot starve HTTP/WiFi.
    for (unsigned i = 0; i < 8 && xQueueReceive(queue, &r, 0) == pdTRUE; ++i) {
      Serial.printf("[%llu ms] %s\n", (unsigned long long)r.ms, r.text);
      if (syslogEnabled && state == WL_CONNECTED) {
        // RFC5424: local0 facility, NILVALUE timestamp, uptime in message.
        char packet[320];
        snprintf(packet, sizeof(packet), "<%d>1 - %s hebrew-clock - - - uptime_ms=%llu %s", 16*8+r.severity, CLOCK_HOSTNAME, (unsigned long long)r.ms, r.text);
        bool ok = udp.beginPacket(syslogAddress, CLOCK_SYSLOG_PORT);
        if (ok) { udp.write((const uint8_t*)packet, strlen(packet)); ok = udp.endPacket() == 1; }
        if (ok) ++syslogSent; else ++syslogFailed;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
bool begin() {
  queue = xQueueCreate(24, sizeof(Record));
  if (!queue) return false;
  log(6, "Boot reset_reason=%d", (int)esp_reset_reason());
  if (xTaskCreate(task, "clock-diagnostics", 6144, nullptr, 1, nullptr) == pdPASS) return true;
  vQueueDelete(queue); queue = nullptr;
  return false;
}
}
