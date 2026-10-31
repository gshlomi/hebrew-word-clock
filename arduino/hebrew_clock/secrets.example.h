#pragma once

// Copy this file to secrets.h (git-ignored), then edit the values below.
// All supported secrets.h settings are listed here. Rebuild and upload over
// USB after changes. Timezone/syslog are initial defaults: saved webpage settings
// override them. Use http://<device-ip>/ to change these without rebuilding.

// Required: credentials for your 2.4 GHz Wi-Fi network.
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"  // Use "" for an open network.

// Optional: local timezone, including automatic daylight-saving transitions.
// Default: Israel. Use a POSIX TZ rule, NOT an IANA name (Asia/Jerusalem).
#define CLOCK_TIMEZONE "IST-2IDT,M3.4.4/26,M10.5.0"
// Other values: UTC "UTC0"
//              New York "EST5EDT,M3.2.0/2,M11.1.0/2"
//              Berlin "CET-1CEST,M3.5.0/2,M10.5.0/3"

// Optional: device hostname used by DHCP, mDNS and in syslog messages.
// Default: "hebrew-clock". Use a short ASCII hostname without spaces.
// Diagnostics are at http://hebrew-clock.local/ (mDNS) or http://<device-ip>/.
#define CLOCK_HOSTNAME "hebrew-clock"

// Optional: remote syslog server's numeric IPv4 address (not a DNS name).
// Default: "" disables forwarding. Example: "192.168.1.50".
// Syslog uses unencrypted UDP, RFC5424, facility local0, on a trusted LAN.
#define CLOCK_SYSLOG_IP ""

// Optional: syslog server's UDP listener port (1-65535). Default: 514.
// Initially ignored while CLOCK_SYSLOG_IP is empty; the webpage has an explicit toggle.
#define CLOCK_SYSLOG_PORT 514

// The four CLOCK_* settings above may be omitted to use config.h defaults.
// Diagnostics HTTP port (80), NTP servers, refresh/heartbeat intervals, and
// display pins/layout are fixed in the firmware, not secrets.h options.
