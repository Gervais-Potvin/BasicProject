#pragma once
#include <RTClib.h>

void initRTC();
bool readRTCBool(DateTime &dt);
bool writeRTC(int h, int m, int s, int d, int mo, int y);
String getTimestamp();
bool rtcBatteryOK();
void readRTC();
void setRTC();
void initRTC();
bool writeRTC(int h, int m, int s, int d, int mo, int y);
void print2(int v);
bool rtcBatteryOK();
void setRTC();

