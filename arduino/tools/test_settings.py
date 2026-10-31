#!/usr/bin/env python3
"""Exercise firmware settings with host NVS/RTOS stubs; no hardware claim."""
from pathlib import Path
import subprocess,tempfile
sketch=Path(__file__).resolve().parents[1]/'hebrew_clock'
with tempfile.TemporaryDirectory() as directory:
    d=Path(directory)
    (d/'Preferences.h').write_text(r'''
#include <string>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cassert>
#include <cstdint>
#include <arpa/inet.h>
using String=std::string;
using SemaphoreHandle_t=int;
constexpr int portMAX_DELAY=0;
#define configASSERT assert
int xSemaphoreCreateMutex(){return 1;}
void xSemaphoreTake(int,int){} void xSemaphoreGive(int){}
size_t strlcpy(char* d,const char* s,size_t n){size_t k=strlen(s);if(n){memcpy(d,s,k<n?k:n-1);d[k<n?k:n-1]=0;}return k;}
struct IPAddress { bool fromString(const char* s){in_addr a;return inet_pton(AF_INET,s,&a)==1;} };
std::string storage; bool failWrite=false;
struct Preferences {
 bool begin(const char*,bool){return true;} void end(){}
 size_t getBytesLength(const char*){return storage.size();}
 size_t getBytes(const char*,void* out,size_t n){memcpy(out,storage.data(),n);return n;}
 size_t putBytes(const char*,const void* in,size_t n){if(failWrite)return 0;storage.assign((const char*)in,n);return n;}
};
''')
    (d/'test.cpp').write_text(r'''
#include "config.h"
#include "settings.h"
int main(){
 Settings::begin(); assert(Settings::values.enabled==0); assert(Settings::values.port==514);
 assert(Settings::ruleFor("Asia/Jerusalem")); assert(!Settings::ruleFor("Invalid/Zone"));
 auto next=Settings::values;strlcpy(next.zone,"Etc/UTC",sizeof(next.zone));
 strlcpy(next.rule,Settings::ruleFor(next.zone),sizeof(next.rule));
 strlcpy(next.ip,"192.168.1.50",sizeof(next.ip));next.enabled=1;next.port=5514;
 assert(Settings::save(next));auto rev=Settings::revision.load();
 time_t now=1767225600;tm actual{};Settings::local(now,&actual);assert(actual.tm_hour==0);
 Settings::values={};Settings::begin();assert(Settings::values.port==5514);assert(Settings::values.enabled==1);
 next.enabled=0;assert(Settings::save(next));assert(!Settings::values.enabled);assert(!strcmp(Settings::values.ip,"192.168.1.50"));
 auto bad=next;bad.port=0;assert(!Settings::save(bad));bad=next;bad.enabled=2;assert(!Settings::save(bad));
 bad=next;strcpy(bad.ip,"garbage");assert(!Settings::save(bad));bad=next;bad.enabled=1;bad.ip[0]=0;assert(!Settings::save(bad));
 bad=next;memset(bad.rule,'x',sizeof(bad.rule));assert(!Settings::save(bad));
 failWrite=true;next.port=123;assert(!Settings::save(next));assert(Settings::values.port==5514);failWrite=false;
 storage="corrupt";Settings::begin();assert(Settings::values.port==514);
 for(const auto& z:TIMEZONES){assert(strlen(z.name)<sizeof(next.zone));assert(strlen(z.rule)<sizeof(next.rule));}
}
''')
    subprocess.run(['c++','-std=c++11','-I',str(d),'-I',str(sketch),str(d/'test.cpp'),'-o',str(d/'test')],check=True)
    subprocess.run([str(d/'test')],check=True)
print('Settings: persistence, UTC application, toggle/address retention, validation, failed writes, corrupt storage and mapping bounds passed')
