#!/usr/bin/env python3
"""Regenerate offline IANA/POSIX table from the vendored MIT-licensed CSV."""
import csv,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
rows=list(csv.reader((root/'third_party/posix_tz_db/zones.csv').open()))
assert len({name for name,rule in rows})==len(rows)
assert all(len(name)<64 and 0<len(rule)<128 for name,rule in rows)
(root/'hebrew_clock/timezones.h').write_text('// Generated from nayarsystems/posix_tz_db zones.csv; see ../third_party/posix_tz_db/LICENSE.\n#pragma once\nstruct TimezoneEntry { const char* name; const char* rule; };\nconst TimezoneEntry TIMEZONES[] = {\n'+''.join('  {'+json.dumps(a)+', '+json.dumps(b)+'},\n' for a,b in rows)+'};\n')
print(f'Generated {len(rows)} timezone entries')
