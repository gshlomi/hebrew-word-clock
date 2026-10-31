# IANA to POSIX mapping

Source: https://github.com/nayarsystems/posix_tz_db (MIT).
`zones.csv` and `LICENSE` retrieved from upstream master on 2026-10-31.
The CSV is vendored: builds do not fetch mutable remote data.

To update after timezone law changes, replace the CSV from upstream, run
`python3 arduino/tools/generate_timezones.py`, test, rebuild and upload.
The mapping represents recurring rules, not full historical IANA transitions.
A saved rule remains pinned across firmware updates; reselect the location and
save on the webpage to apply its newly bundled rule.
