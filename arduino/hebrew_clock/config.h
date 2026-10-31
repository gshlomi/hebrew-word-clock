#pragma once

// Override in secrets.h with a POSIX TZ rule (not an IANA zone name).
// Default: Israel, including automatic daylight-saving transitions.
// POSIX week-4 Thursday + 26 hours expresses the last Friday in March.
#ifndef CLOCK_TIMEZONE
#define CLOCK_TIMEZONE "IST-2IDT,M3.4.4/26,M10.5.0"
#endif

#ifndef CLOCK_HOSTNAME
#define CLOCK_HOSTNAME "hebrew-clock"
#endif
// Numeric IPv4 address; empty disables UDP syslog. No DNS dependency.
#ifndef CLOCK_SYSLOG_IP
#define CLOCK_SYSLOG_IP ""
#endif
#ifndef CLOCK_SYSLOG_PORT
#define CLOCK_SYSLOG_PORT 514
#endif
