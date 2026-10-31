#!/usr/bin/env python3
"""Execute the actual firmware drawClock with a bounds-checking display stub."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
sketch = root / 'arduino/hebrew_clock'
source = (sketch/'hebrew_clock.ino').read_text()
renderer = source[source.index('const WordBitmap* findWord'):source.index('void setup()')]
with tempfile.TemporaryDirectory() as temp:
    path = Path(temp)
    (path/'Arduino.h').write_text('#include <cstdint>\n#include <cstddef>\n#define PROGMEM\n')
    (path/'render.cpp').write_text('''#include <cstdio>
#include <cstring>
#include <cassert>
#include <cstdint>
#define PROGMEM
#define strncpy_P strncpy
#define pgm_read_ptr(x) (*(x))
#include "time_words.h"
#include "word_bitmaps.h"
constexpr int TFT_WHITE=1, TFT_BLACK=0;
namespace Diagnostics {
  bool busy = false;
  int starts = 0, ends = 0;
  void log(int severity, const char*, ...) { assert(severity > 3 && "renderer reported an error"); }
  void refresh(bool start) {
    assert(start != busy); busy = start;
    if (start) ++starts; else ++ends;
  }
}
struct {
  int updates=0, words=0;
  void fillScreen(int) { words=0; }
  void drawBitmap(int x, int y, const uint8_t*, int w, int h, int) {
    assert(x>=40 && x+w<=760 && y>=40 && y+h<=440); ++words;
  }
  void update() { assert(words>0 && Diagnostics::busy); ++updates; }
} display;
''' + renderer + '''
int main() {
  for(int h=0;h<24;++h) for(int m=0;m<60;++m) {
    char text[160]; buildTimeString(h,m,text,sizeof(text));
    drawClock(text); assert(display.updates==h*60+m+1);
  }
  drawClock("אין חיבור"); drawClock("ממתין לשעה");
  assert(display.updates==1442);
  assert(Diagnostics::starts==1442 && Diagnostics::ends==1442 && !Diagnostics::busy);
  puts("Actual firmware renderer: 1440 minutes + 2 status messages fit, no missing words");
}
''')
    subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined','-I',str(path),'-I',str(sketch),str(path/'render.cpp'),'-o',str(path/'render')],check=True)
    subprocess.run([str(path/'render')],check=True)
