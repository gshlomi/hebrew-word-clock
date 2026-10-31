#pragma once
#include <WebServer.h>
#include <ESPmDNS.h>
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
bool mdnsStarted = false;
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
  time_t now = time(nullptr); tm local{}; Settings::local(now, &local);
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
    Settings::values.ip, (unsigned)Settings::values.port, syslogEnabled ? "enabled" : "disabled", syslogSent, syslogFailed, lost);
  return String(buf);
}
const char PAGE[] PROGMEM = R"HTML(<!doctype html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>Hebrew clock diagnostics</title></head><body><h1>Hebrew clock diagnostics</h1><p>Status refreshes every 5 seconds. <a href="/logs">Download logs</a></p><p id="connection"></p><h2>Settings</h2><form id="settings">
<p><label>Timezone <select id="zone" name="zone" required></select></label></p>
<p><label><input type="checkbox" id="enabled" name="enabled"> Enable remote syslog</label></p>
<p><label>Syslog server IPv4 <input id="ip" name="ip" maxlength="15" placeholder="192.168.1.50"></label></p>
<p><label>UDP port <input id="port" name="port" type="number" min="1" max="65535" required></label></p>
<button id="save" disabled>Save settings</button><p id="saved" role="status"></p></form>
<p>Saved on the clock; applies without reboot. Settings access is for a trusted LAN.</p><h2>Status</h2><pre id="status"></pre><h2>Recent logs</h2><pre id="logs"></pre><script>
const el=id=>document.getElementById(id);
let token='';
async function loadSettings(){try{
const [zones,current]=await Promise.all(['/timezones','/settings'].map(async p=>{const r=await fetch(p,{cache:'no-store'});if(!r.ok)throw Error('Load failed');return r.text()}));
const [zone,ip,port,enabled,key]=current.trimEnd().split('\n');token=key;
el('zone').replaceChildren(...zones.trimEnd().split('\n').map(z=>new Option(z,z)));
el('zone').value=zone;el('ip').value=ip;el('port').value=port;el('enabled').checked=enabled==='1';el('save').disabled=false;
}catch(e){el('saved').textContent='Settings unavailable. Reload the page to retry.'}}
el('settings').addEventListener('submit',async e=>{e.preventDefault();el('save').disabled=true;
try{const body=new URLSearchParams({zone:el('zone').value,ip:el('ip').value.trim(),port:el('port').value,enabled:el('enabled').checked?'1':'0'});
const r=await fetch('/settings',{method:'POST',headers:{'X-Clock-Token':token},body});const text=await r.text();el('saved').textContent=text;
}catch(e){el('saved').textContent='Connection lost; reload to check whether settings were saved.'}finally{el('save').disabled=false}});loadSettings();
async function update(){try{const results=await Promise.all(['/status','/logs'].map(async p=>{const r=await fetch(p,{cache:'no-store',signal:AbortSignal.timeout(4000)});if(!r.ok)throw Error(r.status);return r.text()}));document.getElementById('status').textContent=results[0];document.getElementById('logs').textContent=results[1];document.getElementById('connection').textContent='Updated '+new Date().toLocaleTimeString()}catch(e){document.getElementById('connection').textContent='Device unreachable — displayed data is stale.'}finally{setTimeout(update,5000)}}update();
</script></body></html>)HTML";
String settingsToken;
void configureSyslog() {
  udp.stop();
  syslogEnabled = Settings::values.enabled && syslogAddress.fromString(Settings::values.ip);
}
void saveSettings() {
  // A per-boot token with a custom header blocks cross-origin form submissions.
  // Not authentication: the device must remain on a trusted LAN.
  if (server.header("X-Clock-Token") != settingsToken) { server.send(403, "text/plain", "Reload the page before saving."); return; }
  if (!server.hasArg("zone") || !server.hasArg("ip") || !server.hasArg("port") || !server.hasArg("enabled")) {
    server.send(400, "text/plain", "Missing settings."); return;
  }
  String zone=server.arg("zone"), ip=server.arg("ip"), port=server.arg("port"), enabled=server.arg("enabled");
  const char* rule=Settings::ruleFor(zone);
  unsigned long number=0;
  bool digits=port.length()>0 && port.length()<=5;
  for (unsigned i=0;i<port.length();++i) { if (port[i]<'0'||port[i]>'9') digits=false; else number=number*10+port[i]-'0'; }
  IPAddress address;
  if (!rule || zone.length()>=64 || ip.length()>=16 || (ip.length() && !address.fromString(ip)) ||
      (enabled!="0" && enabled!="1") || (enabled=="1" && !ip.length()) || !digits || number<1 || number>65535) {
    server.send(400, "text/plain", "Choose a timezone, valid IPv4 address, and UDP port 1–65535. An address is required when syslog is enabled."); return;
  }
  Settings::Values next{}; next.version=1;
  strlcpy(next.zone,zone.c_str(),sizeof(next.zone)); strlcpy(next.rule,rule,sizeof(next.rule));
  strlcpy(next.ip,ip.c_str(),sizeof(next.ip)); next.port=number; next.enabled=enabled=="1";
  if (!Settings::save(next)) { server.send(500,"text/plain","Could not save settings to flash; existing settings retained."); return; }
  configureSyslog(); log(6,"Runtime settings saved: timezone=%s syslog=%s",next.zone,syslogEnabled?"enabled":"disabled");
  server.send(200,"text/plain","Saved and applied. Display updates when the current refresh finishes.");
}
void task(void*) {
  configureSyslog();
  char token[33]; snprintf(token,sizeof(token),"%08lx%08lx%08lx%08lx",(unsigned long)esp_random(),(unsigned long)esp_random(),(unsigned long)esp_random(),(unsigned long)esp_random()); settingsToken=token;
  const char* headers[]={"X-Clock-Token"}; server.collectHeaders(headers,1);
  server.on("/timezones",HTTP_GET,[] {
    server.setContentLength(CONTENT_LENGTH_UNKNOWN); server.send(200,"text/plain","");
    server.sendContent("Firmware default\n");
    String chunk; chunk.reserve(1024);
    for (const auto& entry:TIMEZONES) { chunk+=entry.name; chunk+='\n'; if(chunk.length()>900){server.sendContent(chunk);chunk="";} }
    server.sendContent(chunk); server.sendContent("");
  });
  server.on("/settings",HTTP_GET,[] {
    server.sendHeader("Cache-Control","no-store");
    server.send(200,"text/plain",String(Settings::values.zone)+"\n"+Settings::values.ip+"\n"+String(Settings::values.port)+"\n"+String(Settings::values.enabled)+"\n"+settingsToken+"\n");
  });
  server.on("/settings",HTTP_POST,saveSettings);
  server.on("/", [] { server.send_P(200, "text/html; charset=utf-8", PAGE); });
  server.on("/status", [] { server.sendHeader("Cache-Control", "no-store"); server.send(200, "text/plain; charset=utf-8", status()); });
  server.on("/logs", [] { server.sendHeader("Cache-Control", "no-store"); server.sendHeader("Content-Disposition", "attachment; filename=clock.log"); server.send(200, "text/plain; charset=utf-8", logs()); });
  server.begin();
  int previous = -1; uint64_t retry = uptime(), heartbeat = 0; bool synced = false;
  for (;;) {
    int state = WiFi.status();
    if (state != previous) {
      log(6, "WiFi status=%d IP=%s RSSI=%d reset_reason=%d", state, WiFi.localIP().toString().c_str(), WiFi.RSSI(), (int)esp_reset_reason()); previous = state;
      if (state == WL_CONNECTED && !mdnsStarted) {
        if (MDNS.begin(CLOCK_HOSTNAME)) {
          MDNS.addService("http", "tcp", 80);
          mdnsStarted = true;
          log(6, "mDNS started: http://%s.local/ IP=%s", CLOCK_HOSTNAME, WiFi.localIP().toString().c_str());
        } else {
          log(4, "mDNS start failed; use http://%s/", WiFi.localIP().toString().c_str());
        }
      } else if (state != WL_CONNECTED && mdnsStarted) {
        MDNS.end(); mdnsStarted = false;
        log(6, "mDNS stopped (WiFi disconnected)");
      }
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
        bool ok = udp.beginPacket(syslogAddress, Settings::values.port);
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
