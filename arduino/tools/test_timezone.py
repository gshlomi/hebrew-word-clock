#!/usr/bin/env python3
"""Check the firmware's POSIX Israel rule against installed IANA timezone data."""
import datetime as dt
import os
from pathlib import Path
import re
import time
from zoneinfo import ZoneInfo

source = (Path(__file__).resolve().parents[1]/'hebrew_clock/hebrew_clock.ino').read_text()
os.environ['TZ'] = re.search(r'ISRAEL_TZ = "([^"]+)"', source)[1]
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
