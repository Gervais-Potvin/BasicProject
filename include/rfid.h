#pragma once
#include <Arduino.h>

String readRFID();
void selectRFID();
void rfidDeselectAll();
void initRFID();
bool isValidRFID(String uid);