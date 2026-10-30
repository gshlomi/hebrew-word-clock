#pragma once

// Override in secrets.h with a POSIX TZ rule (not an IANA zone name).
// Default: Israel, including automatic daylight-saving transitions.
// POSIX week-4 Thursday + 26 hours expresses the last Friday in March.
#ifndef CLOCK_TIMEZONE
#define CLOCK_TIMEZONE "IST-2IDT,M3.4.4/26,M10.5.0"
#endif
