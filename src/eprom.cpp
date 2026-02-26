#include <Arduino.h>
#include <EEPROM.h>
#include "eprom.h"
#include "rfid.h"

void saveUIDToEEPROM(String uid) {
    if (uid.length() == 0) return;

    // 1. Écrire la longueur
    EEPROM.write(EEPROM_UID_ADDR, uid.length());

    // 2. Écrire les caractères
    for (size_t i = 0; i < uid.length() && i < EEPROM_UID_MAX_LEN - 2; i++) {
        EEPROM.write(EEPROM_UID_ADDR + 1 + i, uid[i]);
    }

    // 3. Terminaison
    EEPROM.write(EEPROM_UID_ADDR + 1 + uid.length(), 0);

    Serial.print("EEPROM interne: UID sauvegardé → ");
    Serial.println(uid);
}
void clearUIDFromEEPROM() {
    EEPROM.write(EEPROM_UID_ADDR, 0); // longueur = 0
    for (int i = 1; i < EEPROM_UID_MAX_LEN; i++) {
        EEPROM.write(EEPROM_UID_ADDR + i, 0);
    }
    Serial.println("EEPROM interne: UID effacé");
}
String readUIDFromEEPROM() {
    int len = EEPROM.read(EEPROM_UID_ADDR);

    if (len <= 0 || len > EEPROM_UID_MAX_LEN - 2)
        return "";

    String uid = "";

    for (int i = 0; i < len; i++) {
        char c = EEPROM.read(EEPROM_UID_ADDR + 1 + i);
        if (c == 0) break;
        uid += c;
    }

    if (isValidRFID(uid)) {
        Serial.print("EEPROM interne: UID lu → ");
        Serial.println(uid);
        return uid;
    }

    return "";
}
