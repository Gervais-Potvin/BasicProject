#pragma once

#define EEPROM_UID_ADDR    0
#define EEPROM_UID_MAX_LEN 32   // largement suffisant pour UID RFID

void saveUIDToEEPROM(String uid);
void clearUIDFromEEPROM();
String readUIDFromEEPROM();
