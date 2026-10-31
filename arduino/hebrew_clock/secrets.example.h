#pragma once
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Optional timezone override: uncomment and edit, then rebuild/upload.
// Use a POSIX TZ rule, not an IANA name such as "Asia/Jerusalem".
// Israel is the default when this setting is omitted.
// #define CLOCK_TIMEZONE "IST-2IDT,M3.4.4/26,M10.5.0"
// For UTC instead: #define CLOCK_TIMEZONE "UTC0"

// Read-only diagnostics: http://<device-ip>/ on your trusted LAN.
// Optional remote syslog (UDP, RFC5424, local0). Rebuild/upload after edits.
// #define CLOCK_HOSTNAME "hebrew-clock"
// #define CLOCK_SYSLOG_IP "192.168.1.50"
// #define CLOCK_SYSLOG_PORT 514
