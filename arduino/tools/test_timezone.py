#!/usr/bin/env python3
"""Check the firmware's POSIX Israel rule against installed IANA timezone data."""
import datetime as dt
import os
from pathlib import Path
import subprocess
import tempfile
import time
from zoneinfo import ZoneInfo

sketch = Path(__file__).resolve().parents[1]/'hebrew_clock'
# Compile the same config header used by firmware, including an override.
with tempfile.TemporaryDirectory() as temp:
    source = Path(temp)/'timezone.cpp'
    binary = Path(temp)/'timezone'
    source.write_text('#include <cstdio>\n#include "config.h"\n'
                      'int main() { std::puts(CLOCK_TIMEZONE); }\n')
    def configured_timezone(*flags):
        subprocess.run(['c++', '-I', str(sketch), *flags, str(source),
                        '-o', str(binary)], check=True)
        return subprocess.check_output([str(binary)], text=True).strip()
    default_timezone = configured_timezone()
    override = configured_timezone('-DCLOCK_TIMEZONE="UTC0"')
    assert override == 'UTC0', override
os.environ['TZ'] = override
time.tzset()
for epoch in (1735689600, 1751328000):
    assert time.localtime(epoch)[:6] == time.gmtime(epoch)[:6]
print('Timezone override: UTC0 remains UTC in winter and summer')
os.environ['TZ'] = default_timezone
time.tzset()
zone = ZoneInfo('Asia/Jerusalem')
start = int(dt.datetime(2025,1,1,tzinfo=dt.timezone.utc).timestamp())
end = int(dt.datetime(2036,1,1,tzinfo=dt.timezone.utc).timestamp())
count = 0
for epoch in range(start, end, 1800):
    expected = dt.datetime.fromtimestamp(epoch, zone)
    actual = time.localtime(epoch)
    assert (actual.tm_year, actual.tm_mon, actual.tm_mday, actual.tm_hour, actual.tm_min) == (
        expected.year, expected.month, expected.day, expected.hour, expected.minute), expected
    count += 1
print(f'Israel timezone: {count} half-hour samples match IANA (2025–2035)')
