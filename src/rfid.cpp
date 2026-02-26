#include "rfid.h"
#include <MFRC522.h>
#include "hardware.h" // Projet
#include "buzzer.h" //Projet

MFRC522 rfid(RFID_SS, RFID_RST);

// Exemple : adapte selon ton câblage
extern MFRC522 rfid;
String strLastRFID = ""; // Stocke le dernier RFID lu pour éviter les lectures multiples
String strRFID; // UID de la carte en cours de validation

void selectRFID() {
  // Désélection de SD et Sélection du RFID
  digitalWrite(SD_CS, HIGH);   // SD off
  digitalWrite(RFID_SS, LOW);  // RFID on
}
void rfidDeselectAll() {
  // Désélection de SD et RFID
  digitalWrite(SD_CS, HIGH);
  digitalWrite(RFID_SS, HIGH);
}
void initRFID() {
  pinMode(RFID_SS, OUTPUT);
  selectRFID();
  rfid.PCD_Init();
  rfidDeselectAll();
}
String readRFID() {
  // Sélection du module RFID
  selectRFID();
  // Vérifie si une nouvelle carte est présente
  if (!rfid.PICC_IsNewCardPresent()) {
    rfidDeselectAll();
    return "";
  }
  // Vérifie si on peut lire la carte
  if (!rfid.PICC_ReadCardSerial()) {
    rfidDeselectAll();
    return "";
  }
  // Construction de l’UID
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) uid += ":";
  }
  uid.toUpperCase();
  // Mise à jour globale
  strLastRFID = uid;
  // Petit feedback
  beep(50);
  // Libère le bus SPI
  rfidDeselectAll();
  return uid;
}

bool isValidRFID(String uid) { // Validation du format du UID
  // Trop court → pas un UID
  if (uid.length() < 4) return false;
  // Doit contenir au moins un ':'
  if (uid.indexOf(':') == -1) return false;
  // Vérifier que tous les caractères sont HEX ou ':'
  for (size_t i = 0; i < uid.length(); i++) {
    char c = uid[i];
    bool isHex = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
    if (!isHex && c != ':') return false;
  }
  return true;
}
