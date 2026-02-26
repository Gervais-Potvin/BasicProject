#include "rtc.h"

extern unsigned long ulngLastActivity;

RTC_DS3231 rtc; //Adresse 0x68 pour le DS3231 et Adresse 0x57 pour le EEProm intégré sur le DS3231

String strTimeStamp = "0000/00/00 00:00:00";
String strYear = "0000";

void initRTC() {
  ulngLastActivity = millis();
  rtc.begin();
  getTimestamp(); // Initialisation
}

bool readRTCBool(DateTime &dt) {
  // Lecture de l'heure et de la Date du RTC
  dt = rtc.now();
  return true;
}

void print2(int v) {
  // Format à 2 Digits
  if (v < 10) Serial.print("0");
  Serial.print(v);
}

void readRTC() {
  // Fonction Administrateur de Lecture Date/Heure du RTC via Serial Monitor
  DateTime now = rtc.now();
  Serial.print("RTC: ");
  print2(now.hour());   Serial.print(":");
  print2(now.minute()); Serial.print(":");
  print2(now.second()); Serial.print("  ");
  print2(now.day());    Serial.print("/");
  print2(now.month());  Serial.print("/");
  Serial.println(now.year());
}

bool writeRTC(int h, int m, int s, int d, int mo, int y) {
  // Ajustement de l'heure et de la Date du RTC
  rtc.adjust(DateTime(y, mo, d, h, m, s));
  return true;
}

String getTimestamp() {  // Obtenir le Timestamp
  // Création d'un Timestamp Standardisé
  strTimeStamp = "0000/00/00 00:00:00";
  strYear = "0000";
  DateTime now = rtc.now();
  // Mise à jour de l'année seule
  char ybuf[5];
  sprintf(ybuf, "%04d", now.year());
  strYear = String(ybuf);
  // Mise à jour du timestamp complet
  char buffer[20];
  sprintf(buffer, "%04d/%02d/%02d %02d:%02d:%02d",
          now.year(), now.month(), now.day(),
          now.hour(), now.minute(), now.second());
  strTimeStamp = String(buffer);
  return strTimeStamp;
}

bool rtcBatteryOK(){
  // Validation de la batterie du RTC
  Wire.beginTransmission(0x68);   // Adresse DS3231
  Wire.write(0x0F);               // Registre Status
  Wire.endTransmission();
  Wire.requestFrom(0x68, 1);
  if (!Wire.available()) {
    Serial.println("Erreur lecture registre RTC");
    return false;
  }
  byte status = Wire.read();
  // Bit 7 = OSF (Oscillator Stop Flag)
  bool osf = status & 0x80;
  if (osf) {
    Serial.println("Batterie RTC faible ou absente (OSF=1)");
    return false;
  } else {
    Serial.println("Batterie RTC OK");
    return true;
  }
}
void setRTC() {
  // Fonction Administrateur d'ajustement Date/Heure du RTC via Serial Monitor
  Serial.println("Format: HH MM SS DD MM YYYY");
  while (!Serial.available());
  String line = Serial.readStringUntil('\n');
  int h, m, s, d, mo, y;
  sscanf(line.c_str(), "%d %d %d %d %d %d", &h, &m, &s, &d, &mo, &y);
  writeRTC(h, m, s, d, mo, y);
  Serial.println("Heure mise à jour.");
}
