#include <Arduino.h>
// =========================
// INCLUDE
// =========================
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <EEPROM.h>
// =========================
// PINS
// =========================
#define PIN_BUZZER 5
#define PIN_LED 6
#define PIN_SSR_A 7
#define PIN_SSR_B 12
#define PIN_AC_DETECT A0

#define DIP1 22
#define DIP2 23
#define DIP3 24
#define DIP4 25

#define RFID_SS 4
#define RFID_RST 3
#define SD_CS    10

#define EEPROM_UID_ADDR    0
#define EEPROM_UID_MAX_LEN 32   // largement suffisant pour UID RFID

#define DEBUG 1   // Mets 0 pour désactiver
#if DEBUG
  #define DBG(x)    Serial.print(x)
  #define DBGLN(x)  Serial.println(x)
#else
  #define DBG(x)
  #define DBGLN(x)
#endif

// =========================
// MODULES ELECTRONIQUES EXTERNES
// =========================
MFRC522 rfid(RFID_SS, RFID_RST);
LiquidCrystal_I2C lcd(0x27, 20, 4); // LCD 20x4 adresse 0x27. pour changer l'adresse voir A0-A1-A2 sur le controleur I2C
RTC_DS3231 rtc; //Adresse 0x68 pour le DS3231 et Adresse 0x57 pour le EEProm intégré sur le DS3231
// =========================
// VARIABLES GLOBALES
// =========================
// Informations Globale sur le courant 
const int PIN_SCT013 = A0;   // entrée analogique du SCT013
float currentAmps = 0.0;      // courant estimé
float amps_idle = 12.0; // Charge lue au Primaire de la borne pas de véhicule connecté
float amps_load = 16.0; // Charge lue au Primaire de la borne véhicule connecté
//float currentStartThreshold = 65.7; // seuil en ampères pour dire "charge active"
//float currentStopThreshold = 65; // seuil en ampères pour dire "charge inactive"
float currentStartThreshold = 14.5; // seuil en ampères pour dire "charge active"
float currentStopThreshold = 13.0; // seuil en ampères pour dire "charge inactive"
// Informations Globale sur la carte courante
String gUID = "";
String gLastUID = "";
String lastRFID = "";
String gPrenom = "";
String gNom = "";
String gVehicule = "";
String gTelephone = "";
bool gCardFound = false;
// Variables Globales de temps
String strTimeStamp = "";
String strYear = "";
bool gToggleDisplay = false; // Pulse pour alterner des informations au LCD
unsigned long ssrStartTime = 0;   // moment où les SSR ont été activés
unsigned long ssrAccumulated = 0; // temps total accumulé en millisecondes
double ssrCostPerHour = 2.0; // taux horaire de la charge
float gRunTime = 0.0;           // temps total formaté en minutes
float gRunTimeHr = 0.0;         // temps total formaté en heures
float gRunCost = 0.0;           // temps total formaté en dollars
String strRuntime = "";           // temps total formaté en minutes
String strRuntimeHr = "";         // temps total formaté en heures
String strRunCost = "";           // temps total formaté en dollars
bool ssrIsActive = false;         // état actuel des SSR
// =========================
// VARIABLES GLOBALES LCD
// =========================
unsigned long lastLCDUpdate = 0;
const unsigned long LCD_UPDATE_INTERVAL = 3000; // en ms
// --- LCD Rest Mode ---
bool lcdRestMode = false;
unsigned long lastLCDRestActivity = 0;
const unsigned long LCD_REST_TIMEOUT = 300000;      // 300 sec avant repos ==> 300000 msec
const unsigned long LCD_REST_REFRESH = 2000;       // rafraîchissement minimal
unsigned long lastLCDRestRefresh = 0;
unsigned long lastRestBlink = 0;
const unsigned long REST_BLINK_INTERVAL = 30000; // 30 sec

// =========================
// VARIABLES GLOBALES MACHINE ÉTAT
// =========================
enum State {
  ATTENTE_CARTE,
  VALIDATION_CARTE,
  CARTE_VALIDE,       // État instantané
  CARTE_INVALIDE,     // État instantané
  ATTENTE_CONNEXION,
  CHARGE_EN_COURS,
  FIN_CHARGE,
  MODE_ADMIN
};
State state = ATTENTE_CARTE;
unsigned long lastActivity = 0;
String cmd = "";
String gQuitMode = "";
bool entry_AttenteCarte = true;
bool entry_ValidationCarte = true;
bool entry_CarteValide = true;
bool entry_CarteInvalide = true;
bool entry_AttenteConnexion = true;
bool entry_ChargeEnCours = true;
bool entry_FinCharge = true;
bool entry_ModeAdmin = true;

// =========================
// FONCTIONS UTILITAIRE GENERAL
// =========================
void print2(int v) {
  // Format à 2 Digits
  if (v < 10) Serial.print("0");
  Serial.print(v);
}
// =========================
// FONCTIONS RTC — DS3231 - Gestion du Temps
// =========================
void lcdUpdateClock() {
    lcd.setCursor(0, 0);
    lcd.print(getTimestamp());
}
bool readRTC(DateTime &dt) {
  // Lecture de l'heure et de la Date du RTC
  dt = rtc.now();
  return true;
}
bool writeRTC(int h, int m, int s, int d, int mo, int y) {
  // Ajustement de l'heure et de la Date du RTC
  rtc.adjust(DateTime(y, mo, d, h, m, s));
  return true;
}
bool rtcBatteryOK() {
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
void updateRuntime() {
  // Mise à jour du temps de fonctionnement des SolidState Relays
  unsigned long elapsed = millis() - ssrStartTime;
  gRunTime = elapsed / 60000UL;  // minutes écoulées
  gRunCost = (gRunTime / 60.0) * ssrCostPerHour;
  gRunTimeHr = gRunTime / 60UL;

  strRuntime = String(gRunTime, 0);
  strRuntimeHr = String(gRunTimeHr, 2);
  strRunCost = String(gRunCost, 2); 
}
void resetActivityTimer() {
    lastActivity = millis();
}
bool isInactive(unsigned long timeoutMs) {  // Retourne True si on est inactif depuis un certain temps
    unsigned long elapsed = millis() - lastActivity;
    unsigned long elapsedSec = elapsed / 1000;
    unsigned long timeoutSec = timeoutMs / 1000;
    DBG("Inactivité : ");
    DBG(elapsedSec);
    DBG("/");
    DBG(timeoutSec);
    DBGLN(" sec");
    return elapsed > timeoutMs;
}
// =========================
// FONCTIONS GESTION EEPROM
// =========================
void saveUIDToEEPROM(String uid) {
    if (uid.length() == 0) return;

    // 1. Écrire la longueur
    EEPROM.write(EEPROM_UID_ADDR, uid.length());

    // 2. Écrire les caractères
    for (int i = 0; i < uid.length() && i < EEPROM_UID_MAX_LEN - 2; i++) {
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

    if (isValidUID(uid)) {
        Serial.print("EEPROM interne: UID lu → ");
        Serial.println(uid);
        return uid;
    }

    return "";
}
bool isValidUID(String uid) { // Validation du format du UID
  // Trop court → pas un UID
  if (uid.length() < 4) return false;
  // Doit contenir au moins un ':'
  if (uid.indexOf(':') == -1) return false;
  // Vérifier que tous les caractères sont HEX ou ':'
  for (int i = 0; i < uid.length(); i++) {
    char c = uid[i];
    bool isHex = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
    if (!isHex && c != ':') return false;
  }
  return true;
}// =========================
// FONCTIONS GESTION HARDWARE ARDUINO
// =========================
void spiDeselectAll() {
  // Désélection de SD et RFID
  digitalWrite(SD_CS, HIGH);
  digitalWrite(RFID_SS, HIGH);
}
void selectRFID() {
  // Désélection de SD et Sélection du RFID
  digitalWrite(SD_CS, HIGH);   // SD off
  digitalWrite(RFID_SS, LOW);  // RFID on
}
void selectSD() {
  // Sélection de SD et Désélection du RFID
  digitalWrite(RFID_SS, HIGH); // RFID off
  digitalWrite(SD_CS, LOW);    // SD on
}
void beep(int durationMs) {
  // Buzzer pendant un délai en millisecondes
  digitalWrite(PIN_BUZZER, HIGH);
  delay(durationMs);
  digitalWrite(PIN_BUZZER, LOW);
}
void ssrOn() {
  // Activation simultanée des 2 SolidState Relays
  digitalWrite(PIN_SSR_A, HIGH);
  digitalWrite(PIN_SSR_B, HIGH);
  if (!ssrIsActive) {
    ssrStartTime = millis();
    ssrIsActive = true;
  }
}
void ssrOff() {
  // Désactivation simultanée des 2 SolidState Relays
  digitalWrite(PIN_SSR_A, LOW);
  digitalWrite(PIN_SSR_B, LOW);
  if (ssrIsActive) {
    ssrAccumulated += millis() - ssrStartTime;
    ssrIsActive = false;
  }
}
// =========================
// FONCTIONS GESTION DU SD
// =========================
bool checkSD() {
  // Validation du fonctionnement de base de la carte SD
  selectSD();
  if (!SD.begin(SD_CS)) {
    Serial.println("SD FAIL");
    return false;
  }
  File f = SD.open("log.txt", FILE_WRITE);
  if (f) {
    f.println("Test SD OK");
    f.close();
    Serial.println("Écriture SD OK");
  } else {
    Serial.println("Impossible d'ouvrir log.txt");
    return false;
  }
  spiDeselectAll();
  return true;
}
void LogEvent(String type, String message) {
  // Sélection du module SD
  selectSD();
  if (!SD.begin(SD_CS)) {
    Serial.println("SD ERROR: Impossible d'initialiser la carte SD");
    spiDeselectAll();
    return;
  }
  // Nom du fichier basé sur l'année courante
  String filename = "Log" + strYear + ".csv";
  File f = SD.open(filename, FILE_WRITE);
  if (!f) {
    Serial.print("SD ERROR: Impossible d'ouvrir ");
    Serial.println(filename);
    spiDeselectAll();
    return;
  }
  // Construction de la ligne CSV
  String line = "";
  line += getTimestamp(); line += ",";
  line += type;          line += ",";
  line += gUID;          line += ",";
  line += gPrenom;       line += ",";
  line += gNom;          line += ",";
  line += message;
  f.println(line);
  f.close();
  spiDeselectAll();
  Serial.print("LOG EVENT → ");
  Serial.println(line);
}
void Log2SD(String timestamp, String uid, String info1, String info2, String info3) {
  // Sélection du module SD
  selectSD();
  // Vérifie que la SD est prête
  if (!SD.begin(SD_CS)) {
    Serial.println("SD ERROR: Impossible d'initialiser la carte SD");
    spiDeselectAll();
    return;
  }
  // Nom du fichier basé sur l'année courante
  String filename = "Log" + strYear + ".csv";
  // Ouvre le fichier en mode append
  File f = SD.open(filename, FILE_WRITE);
  if (!f) {
    Serial.print("SD ERROR: Impossible d'ouvrir ");
    Serial.println(filename);
    spiDeselectAll();
    return;
  }
  // Construction de la ligne CSV
  String line = "";
  line += timestamp; 
  line += ",";
  line += uid;       
  line += ",";
  line += info1;    
  line += ",";
  line += info2;       
  line += ",";
  line += info3;
  // Écriture
  f.println(line);
  f.close();
  // Libère le bus SPI
  spiDeselectAll();
  Serial.print("SD LOG OK → ");
  Serial.println(line);
}
// =========================
// FONCTIONS GESTION DU SCT013
// =========================
bool checkStopByCurrent() {
  if (!ssrIsActive) return false;   // SSR OFF → pas de charge en cours
  float amps = readSCT();
  // Détection fin de charge
  if (amps < currentStopThreshold) {
    Serial.print("Fin de charge détectée : ");
    Serial.print(amps);
    Serial.println(" A");
    resetActivityTimer();
    return true;
  }
  return false;
}
bool checkStartByCurrent() {
  if (!ssrIsActive) return false;   // SSR OFF → pas de charge possible
  float amps = readSCT();
  // Détection début de charge
  if (amps > currentStartThreshold) {
    Serial.print("Début de charge détecté : ");
    Serial.print(amps);
    Serial.println(" A");
    resetActivityTimer();
    return true;
  }
  return false;
}
bool isCharging() {  // True si on a le courant lu à l'entrée de la borne dépasse un seul minimum 
  if (!ssrIsActive) return false;
  float amps = readSCT();
  return amps > currentStartThreshold;
}
float readSCT() {
    const int samples = 1000;
    long sum = 0;

    for (int i = 0; i < samples; i++) {
        int raw = analogRead(A0);   // 0 à 1023
        sum += (long)raw * (long)raw;
    }

    float mean = sum / (float)samples;
    float rmsCounts = sqrt(mean);

    // ADC → volts
    float volts = rmsCounts * (5.0 / 1023.0);

    // SCT013-030 : 1 V = 30 A
    float amps = volts * 30.0;

    return amps;
}
// =========================
// FONCTIONS SETUP INTERACTIF VIA SERIAL COMMAND
// =========================
String readLine() {
  while (!Serial.available());
  String s = Serial.readStringUntil('\n');
  s.trim();  // enlève \r, \n, espaces
  return s;
}
String readSerialString() {
  // Lecture de string provenant du SerialMonitor
  while (!Serial.available()) { }
  return Serial.readStringUntil('\n');
}
void handleSerialCommands() {
  // Fonction Administrateur Écoute du Serial Monitor pour le traitement des commandes Administrateur
  if (!Serial.available()) return;
  resetActivityTimer();
  cmd = Serial.readStringUntil('\n');
  cmd.trim();
  // --- Activation du mode admin ---
  if (cmd == "admin") {
    state = MODE_ADMIN;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("== MODE ADMIN ==");
    lcd.setCursor(0, 2);
    lcd.print("Console active");
    Serial.println("=== BORNE AMICALE — Fonctions réservées à l'Administration ===");
    Serial.println(" rtc      → Lire l'heure");
    Serial.println(" set      → Définir l'heure");
    Serial.println(" addcard  → Enregistrer une nouvelle carte RFID");
    Serial.println(" delcard  → Effacer une carte RFID du Systeme");
    Serial.println(" findcard → Trouver une carte RFID dans la liste");
    Serial.println(" exit     → Quitter le Mode Administrateur");
    Serial.println("---------------------------------------------------------------");
    return;
  }
  // --- Sortie du mode admin ---
  if (cmd == "exit") {
    state = ATTENTE_CARTE;
    entry_AttenteCarte = true;
    entry_ModeAdmin = true;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sortie Admin...");
    delay(1000);
    Serial.println("Retour au mode normal.");
    return;
  }
  // --- Commandes admin disponibles UNIQUEMENT en mode admin ---
  if (state == MODE_ADMIN) {
    if (cmd == "rtc") readRTC();
    else if (cmd == "set") setRTC();
    else if (cmd == "addcard") addCard();
    else if (cmd == "delcard") delCard();
    else if (cmd == "findcard") {
      if (findCard("")) Serial.println("Carte valide !");
      else Serial.println("Carte inconnue !");
    }
    else Serial.println("Commande inconnue en mode admin.");
    return;
  }
  // --- Si on n'est PAS en mode admin ---
  Serial.println("Commande non disponible hors mode admin.");
}
// =========================
// FONCTIONS GESTION DES CARTES RFID
// =========================
String readRFID() {
  // Sélection du module RFID
  selectRFID();
  // Vérifie si une nouvelle carte est présente
  if (!rfid.PICC_IsNewCardPresent()) {
    spiDeselectAll();
    return "";
  }
  // Vérifie si on peut lire la carte
  if (!rfid.PICC_ReadCardSerial()) {
    spiDeselectAll();
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
  lastRFID = uid;
  // Petit feedback
  beep(50);
  // Libère le bus SPI
  spiDeselectAll();
  return uid;
}
void addCard() {
  // Fonction Administrateur Ajout des informations relatives à une carte RFID sur la carte SD via Serial Monitor
  String uid = readRFID();
  // --- 2. DEMANDE PRÉNOM ---
  Serial.print("Prénom : ");
  String prenom = readSerialString();
  // --- 3. DEMANDE NOM ---
  Serial.print("Nom : ");
  String nom = readSerialString();
  // --- 4. DEMANDE VÉHICULE ---
  Serial.print("Véhicule : ");
  String vehicule = readSerialString();
  // --- 5. DEMANDE TELEPHONE ---
  Serial.print("Téléphone : ");
  String telephone = readSerialString();
  // --- 5. ENREGISTREMENT SUR SD ---
  selectSD();
  if (!SD.begin(SD_CS)) {
    Serial.println("Erreur SD");
    spiDeselectAll();
    return;
  }
  String filename = "Card" + strYear + ".csv";
  File f = SD.open(filename, FILE_WRITE);
  if (!f) {
    Serial.print("Impossible d'ouvrir ");
    Serial.println(filename);
    spiDeselectAll();
    return;
  }
  // Construction de la ligne CSV
  String line = "";
  // --- Ajout du Timestamp ---
  line += getTimestamp();
  line += ",";
  line += uid;
  line += ",";
  line += prenom;
  line += ",";
  line += nom;
  line += ",";
  line += vehicule;
  line += ",";
  line += telephone;
  f.println(line);
  f.close();
  spiDeselectAll();
  Serial.println("Carte ajoutée avec succès !");
  LogEvent("ADMIN", "Carte Ajouté" + uid);
}
void delCard() {
  // Fonction Administrateur Effacement des informations relatives à une carte RFID de la carte SD via Serial Monitor
  String uid = readRFID();
  // --- 2. OUVERTURE SD ---
  selectSD();
  if (!SD.begin(SD_CS)) {
    Serial.println("Erreur SD");
    spiDeselectAll();
    return;
  }
  String filename = "Card" + strYear + ".csv";
  File original = SD.open(filename, FILE_READ);
  if (!original) {
    Serial.print(filename);
    Serial.println(".csv introuvable");
    spiDeselectAll();
    return;
  }
  File temp = SD.open("temp.csv", FILE_WRITE);
  if (!temp) {
    Serial.println("Impossible de créer temp.csv");
    original.close();
    spiDeselectAll();
    return;
  }
  // --- 3. COPIE DES LIGNES SAUF CELLE À SUPPRIMER ---
  bool found = false;
  while (original.available()) {
    String line = original.readStringUntil('\n');
    // Si la ligne contient l’UID → on la saute
    if (line.indexOf(uid) != -1) {
      found = true;
      continue;
    }
    temp.println(line);
  }
  original.close();
  temp.close();
  // --- 4. REMPLACEMENT DU FICHIER ---
  SD.remove(filename);      // On supprime l'ancien
  File newfile = SD.open(filename, FILE_WRITE);
  File temp2 = SD.open("temp.csv", FILE_READ);
  while (temp2.available()) {
    newfile.write(temp2.read());
  }
  newfile.close();
  temp2.close();
  SD.remove("temp.csv");          // On supprime le fichier temporaire
  spiDeselectAll();
  // --- 5. MESSAGE FINAL ---
  if (found) {
    Serial.println("Carte supprimée avec succès !");
  } else {
    Serial.println("UID introuvable dans Cardlist.csv");
  }
  LogEvent("ADMIN", "Carte Supprimée " + uid);
}
bool findCard(String uidParam) {
  // Recherche de la présence des informations d'un UID d'une carte RFID sur la carte SD
  gCardFound = false;
  gUID = "";
  gPrenom = "";
  gNom = "";
  gVehicule = "";
  gTelephone = "";
  String uid = uidParam;
  // --- 1. SI UID VIDE → DEMANDER UNE CARTE ---
  if (uid.length() == 0) {
    uid = readRFID();
  }
  selectSD();
  if (!SD.begin(SD_CS)) {
    Serial.println("Erreur SD");
    spiDeselectAll();
    return false;
  }
  String filename = "Card" + strYear + ".csv";
  File f = SD.open(filename, FILE_READ);
  if (!f) {
    Serial.print(filename);
    Serial.println(" introuvable");
    spiDeselectAll();
    return false;
  }
  // --- 3. PARCOURS DU FICHIER ---
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    // Vérifie si la ligne contient l'UID
    if (line.indexOf(uid) != -1) {
      // Format :
      // date,UID,prenom,nom,vehicule,telephone
      int p1 = line.indexOf(',');               // fin date
      int p2 = line.indexOf(',', p1 + 1);       // fin UID
      int p3 = line.indexOf(',', p2 + 1);       // fin prénom
      int p4 = line.indexOf(',', p3 + 1);       // fin vehicule
      int p5 = line.indexOf(',', p4 + 1);       // fin telephone
      if (p1 > 0 && p2 > 0 && p3 > 0 && p4 > 0 && p5 > 0) {
        gUID      = line.substring(p1 + 1, p2);
        gPrenom   = line.substring(p2 + 1, p3);
        gNom      = line.substring(p3 + 1, p4);
        gVehicule = line.substring(p4 + 1, p5);
        gTelephone = line.substring(p5 + 1);
        gUID.trim();
        gPrenom.trim();
        gNom.trim();
        gVehicule.trim();
        gTelephone.trim();
        gCardFound = true;
      }
      break;
    }
  }
  f.close();
  spiDeselectAll();
  LogEvent("OPERATION", "Recherche Carte " + uid);
  // --- 4. RETOURNE LE RÉSULTAT ---
  return gCardFound;
}
// =========================
// LCD
// =========================
void lcdAccueil() { // Étape initiale Attente de RFID
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(getTimestamp());
  lcd.setCursor(0, 1);
  lcd.print("Suivez instructions");
  lcd.setCursor(0, 2);
  lcd.print("en majuscules...");
  lcd.setCursor(0, 3);
  lcd.print("PASSEZ VOTRE CARTE");
}
void lcdValidationCarte() { // Validation de la carte RFID
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(getTimestamp());
  lcd.setCursor(0, 1);
  lcd.print("Validation Carte");
  lcd.setCursor(0, 2);
  lcd.print("UID:");
  lcd.print(gUID);
  lcd.setCursor(0, 3);
  lcd.print("Veuillez patienter");
}
void lcdCarteValide() { // Carte Valide
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Carte Valide");
  lcd.setCursor(0, 1);
  lcd.print(gPrenom + " " + gNom);
  lcd.setCursor(0, 2);
  lcd.print("Tel:" + gTelephone);
  lcd.setCursor(0, 3);
  lcd.print("BRANCHEZ VEHICULE");
}
void lcdCarteInvalide() { // Carte Invalide
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Carte Invalide");
  lcd.setCursor(0, 1);
  lcd.print("Acces Interdit");
  lcd.setCursor(0, 2);
  lcd.print("CONTACTEZ GERVAIS");
  lcd.setCursor(0, 3);
  lcd.print("AU 418-818-7818");
}
void lcdAttenteConnexion() { // Attente de Branchement du Véhicule
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(getTimestamp());
  lcd.setCursor(0, 1);
  lcd.print(gPrenom + " " + gNom);
  lcd.setCursor(0, 2);
  lcd.print("5 MIN. POUR");
  lcd.setCursor(0, 3);
  lcd.print("BRANCHER VEHICULE");
}
void lcdChargeEnCours() { //Charge en Cours
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(getTimestamp());
  // Ligne 1,2,3 mise à jour dynamique dans fctEtape6()
}
void lcdFinCharge() { // Fin de Charge détectée
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(getTimestamp());
  lcd.setCursor(0, 1);
  lcd.print("Fin de la charge");
  lcd.setCursor(0, 2);
  lcd.print("Cout:$");
  lcd.print(strRunCost);
  lcd.setCursor(0, 3);
  lcd.print("Temps:");
  lcd.print(strRuntime);
  lcd.print("m");
}
void lcdAdmin() { // Mode Administration
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(getTimestamp());
  lcd.setCursor(0, 1);
  lcd.print("MODE ADMIN");
  lcd.setCursor(0, 2);
  lcd.print("Console active");
  lcd.setCursor(0, 3);
  lcd.print("En attente cmd");
}
void lcdClearLine(uint8_t line) {
  lcd.setCursor(0, line);
  lcd.print("                    "); // 20 espaces pour un LCD 20x4
}
void updateLCDRestMode() {
  // --- 1. Entrée en repos ---
  if (!lcdRestMode && (millis() - lastLCDRestActivity > LCD_REST_TIMEOUT)) {
    lcdRestMode = true;
    lcd.clear();
    lcd.noBacklight();            // <<< extinction rétroéclairage
    lastLCDRestRefresh = millis();
    return;
  }
  // --- 2. Mode repos actif ---
  if (lcdRestMode) {
    lcd.noBacklight();            // <<< rétroéclairage toujours OFF en repos
    // Animation anti burn-in toutes les 10 secondes
    if (millis() - lastLCDRestRefresh > 10000) {
      static int pos = 0;
      static int direction = 1;
      lcd.backlight();            // <<< allume juste pour afficher l’animation
      lcd.clear();
      // Message qui se déplace verticalement
      lcd.setCursor(0, pos);
      lcd.print("Pour sortir du Repos");
      lcd.setCursor(0, pos + 1);
      lcd.print("PASSEZ UNE CARTE");
      // Mouvement vertical
      pos += direction;
      if (pos >= 2) direction = -1;
      if (pos <= 0) direction = 1;
      delay(2000);                 // <<< visible brièvement
      lcd.noBacklight();          // <<< retourne en extinction
      lastLCDRestRefresh = millis();
    }
    return;
  }
}
// =========================
// SETUP
// =========================
void setup() {
  Serial.begin(115200);

  // --- Initialisation des timers LCD ---
  lcdRestMode = false;
  lastLCDRestActivity = millis();
  lastLCDRestRefresh = millis();

  resetActivityTimer();

  // --- Initialisation des I/O ---
  pinMode(SD_CS, OUTPUT);
  pinMode(RFID_SS, OUTPUT);
  pinMode(53, OUTPUT); // SS matériel du Mega
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_SSR_A, OUTPUT);
  pinMode(PIN_SSR_B, OUTPUT);
  pinMode(PIN_AC_DETECT, INPUT);
  pinMode(DIP1, INPUT_PULLUP);
  pinMode(DIP2, INPUT_PULLUP);
  pinMode(DIP3, INPUT_PULLUP);
  pinMode(DIP4, INPUT_PULLUP);

  spiDeselectAll();    // Sécurise le bus SPI

  Wire.begin();
  SPI.begin();

  // Initialisation du RTC
  rtc.begin();
  rtcBatteryOK();

  // Initialisation du LCD
  lcd.init(); 
  lcd.backlight();
  // --- INITIALISATION RFID ---
  selectRFID();
  rfid.PCD_Init();
  spiDeselectAll();
  // --- INITIALISATION SD ---
  selectSD();
  SD.begin(SD_CS);
  spiDeselectAll();

  Serial.println("=== BORNE AMICALE — Fonctions Sérielles réservé Administration ===");
  Serial.println(" admin    → Passer en mode Administrateur");
  Serial.println("----------------------------------------");

  getTimestamp(); // Initialisation
  LogEvent("BOOT", "Redémarrage du système");

  // --- Lecture EEPROM pour reprise apres perte de tension ou reset ---
  String resumeUID = readUIDFromEEPROM();
  DBG("ResumeUID: ");
  DBGLN(resumeUID);
  if (isValidUID(resumeUID)) {
    gUID = resumeUID;
    // Activer SSR pour permettre la détection de courant
    ssrOn();
    delay(5000);  // temps pour stabiliser le courant
    if (checkStartByCurrent()) {
      // Recharger les infos de la carte depuis la SD
      if (!findCard(gUID)) {
        Serial.println("ERREUR: UID valide mais introuvable dans la SD !");
        // Sécurité : on annule la reprise
        ssrOff();
        clearUIDFromEEPROM();
        state = ATTENTE_CARTE;
        entry_AttenteCarte = true;
        return;
      }
      state = CHARGE_EN_COURS;
      entry_ChargeEnCours = true;
      Serial.println("Reprise directe de la charge depuis EEPROM.");
    } 
    else {
      ssrOff();
      state = ATTENTE_CONNEXION;
      entry_AttenteConnexion = true;
      Serial.println("Reprise UID valide → attente connexion.");
    }
  } else {
    clearUIDFromEEPROM();
    state = ATTENTE_CARTE;
    entry_AttenteCarte = true;
    Serial.println("EEPROM invalide → nettoyage.");
  }
  DBG("Mode Admin");
  DBG("EEPROM UID au boot = [");
  DBG(resumeUID);
  DBG("]");
}
// =========================
// LOOP
// =========================
void loop() {
  // Gestion du LCD Rest Mode (indépendant)
  updateLCDRestMode();
  // 1. Exécuter les actions de l’étape courante
  runStateActions();
  // 2. Gérer les transitions entre étapes
  handleTransitions();
}
// =========================
// FONCTIONS ETAPES MACHINE D'ÉTATS
// =========================
void runStateActions() {
  switch(state) {
    case ATTENTE_CARTE:
      fctEtape1();
      break;
    case VALIDATION_CARTE:
      fctEtape2();
      break;
    case CARTE_VALIDE:
      fctEtape3();
      break;
    case CARTE_INVALIDE:
      fctEtape4();
      break;
    case ATTENTE_CONNEXION:
      fctEtape5();
      break;
    case CHARGE_EN_COURS:
      fctEtape6();
      break;
    case FIN_CHARGE:
      fctEtape7();
      break;
    case MODE_ADMIN:
      fctEtape8();
      break;
  }
}
void handleTransitions() {
  switch (state) {
    case ATTENTE_CARTE:
      if (gUID != "") {
        state = VALIDATION_CARTE;
        entry_ValidationCarte = true;
      }
      break;
    case VALIDATION_CARTE:
      if (findCard(gUID)) {
        state = CARTE_VALIDE;
        entry_CarteValide = true;
      } else {
        state = CARTE_INVALIDE;
        entry_CarteInvalide = true;
      }
      break;
    case CARTE_VALIDE:
      resetActivityTimer();
      state = ATTENTE_CONNEXION;
      entry_AttenteConnexion = true;
      break;
    case CARTE_INVALIDE:
      resetActivityTimer();
      state = ATTENTE_CARTE;
      entry_AttenteCarte = true;
      break;
    case ATTENTE_CONNEXION:
      // 1. Activer SSR pour tester la présence du véhicule
      ssrOn();  //  Activation des SSR pour permettre la lecture de courant
      delay(5000); // Stabiliser le courant
      if (checkStartByCurrent()) {
        state = CHARGE_EN_COURS;
        entry_ChargeEnCours = true;
      }
      else if (isInactive(5UL * 60UL * 1000UL)) { //5 Minutes
        state = ATTENTE_CARTE;
        entry_AttenteCarte = true;
      }
      break;
    case CHARGE_EN_COURS:
      // Lecture RFID
      String uid = readRFID();
      if (uid != "") {
        lastRFID = uid;
      }
      if (lastRFID == gUID && lastRFID != "") {
        gQuitMode = "Arret par Carte";
        state = FIN_CHARGE;
        entry_FinCharge = true;
      }
      else if (checkStopByCurrent()) {
        gQuitMode = "Arret par Deconnexion";
        state = FIN_CHARGE;
        entry_FinCharge = true;
      }
      break;
    case FIN_CHARGE:
      if (isInactive(5UL * 1000UL)) {
        state = ATTENTE_CARTE;
        entry_AttenteCarte = true;
        entry_FinCharge = true;
      }
      break;
    case MODE_ADMIN:
      if (isInactive(15UL * 60UL * 1000UL)) {
        state = ATTENTE_CARTE;
        entry_AttenteCarte = true;
      }
      break;
  }
}
void fctEtape1() { // Étape initiale Attente de RFID
  // --- REST MODE : LCD en repos ---
  if (lcdRestMode) {
    String uid = readRFID();
    // Si une carte est passée → réveil
    if (uid != "") {
      lcdRestMode = false;
      lastRFID = "";
      resetActivityTimer();
      lastLCDRestActivity = millis();
      return;
    }
    return;   // <<< OBLIGATOIRE : empêche tout affichage LCD
  }
  if (entry_AttenteCarte) { // OneShot
    DBGLN("Attente de Carte");
    lcdAccueil();
    entry_AttenteCarte = false;
    gUID = "";
    lastRFID = "";
    resetActivityTimer();
  }
  String uid = readRFID();
  if (uid != "") {
    gUID = uid;
    resetActivityTimer();
  }
  handleSerialCommands();  // Commandes admin
  lcdUpdateClock(); // Rafraichissement de la Ligne d'horloge du LCD
}
void fctEtape2() { // Validation de la carte RFID
  if (entry_ValidationCarte) { // OneShot
    DBGLN("Validation de Carte");
    lcdValidationCarte();
    entry_ValidationCarte = false;
  }
}
void fctEtape3() { // Carte Valide
  if (entry_CarteValide) { // OneShot
    DBGLN("Carte Valide");
    lcdCarteValide();
    lastRFID = "";
    resetActivityTimer();
    entry_CarteValide = false;
  }
  delay(3000); // Petit feedback visuel
}
void fctEtape4() { // Carte Invalide
  if (entry_CarteInvalide) { // OneShot
    DBGLN("Carte Invalide");
    lcdCarteInvalide();
    resetActivityTimer();
    LogEvent("RFID_FAIL", "Carte inconnue");
    entry_CarteInvalide = false;
  }
  delay(5000); // Petit feedback visuel
}
void fctEtape5() { // Attente de Branchement du Véhicule
  if (entry_AttenteConnexion) { // OneShot
    DBGLN("Attente Connexion");
    lcdAttenteConnexion();
    entry_AttenteConnexion = false;
  }
  // Lecture RFID (pour arrêt manuel parfait)
  lcdUpdateClock(); // Rafraichissement de la Ligne d'horloge du LCD
  readRFID();
}
void fctEtape6() { // Charge en Cours
  // --- OneShot ---
  if (entry_ChargeEnCours) {
    DBGLN("Charge En Cours");
    ssrOn();
    lcdChargeEnCours();
    digitalWrite(PIN_LED, HIGH);   // <<< LED ON pendant la charge
    entry_ChargeEnCours = false;
    lastRFID = "";
    ssrStartTime = millis();
    gRunTime = 0;
    gRunCost = 0.0;
    strRuntime = "0";
    strRunCost = "0.00";
    saveUIDToEEPROM(gUID);   // UID propriétaire

    String resumeUID = readUIDFromEEPROM();
    DBG("Relecture UID: ");
    DBGLN(resumeUID);
  }
  // --- REST MODE : LCD en repos ---
  if (lcdRestMode) {
    String uid = readRFID();
    // Si une carte est passée → réveil de l'écran
    if (uid != "") {
      lcdRestMode = false;       // sortir du repos
      lastRFID = "";             // IMPORTANT : ignorer cette carte
      resetActivityTimer();      // activité détectée
      lastLCDRestActivity = millis();  // reset du timer LCD
      return;                    // ne rien faire d'autre ce cycle
    }
    // Continuer la charge normalement
    updateRuntime();
    return;
  }
  // --- LCD ACTIF : logique normale ---
  lcdUpdateClock();   // rafraîchissement horloge
  String uid = readRFID();   // lecture RFID
  // --- Gestion RFID ---
  if (uid != "") {
    resetActivityTimer();   // activité détectée
    // 1. Carte propriétaire → arrêt de charge
    if (uid == gUID) {
      gQuitMode = "Arret par Carte";
      state = FIN_CHARGE;
      entry_FinCharge = true;
      return;
    }
    // 2. Carte étrangère → ignorer, mais ne pas arrêter
    // (aucune action)
  }
  // --- Lecture courant ---
  float amps = readSCT();
  // --- Mise à jour runtime + coût ---
  updateRuntime();
  // --- Affichage dynamique ---
  if (millis() - lastLCDUpdate >= LCD_UPDATE_INTERVAL) {
    lcdClearLine(1);
    lcdClearLine(2);
    if (gToggleDisplay) {
      lcd.setCursor(0, 1);
      lcd.print("PASSEZ VOTRE CARTE");
      lcd.setCursor(0, 2);
      lcd.print("POUR ARRETER CHARGE");
    } else {
      lcd.setCursor(0, 1);
      lcd.print("OU CONTACTEZ");
      lcd.setCursor(0, 2);
      lcd.print("LE " + gTelephone);
    }
    lcd.setCursor(0, 3);
    lcd.print("Cout:$");
    lcd.print(strRunCost);
    lcd.print("  ");
    if (gRunTime < 60) {
      lcd.print(strRuntime);
      lcd.print(" min.");
    } else {
      lcd.print(strRuntimeHr);
      lcd.print(" hr.");
    }
    lastLCDUpdate = millis();
    gToggleDisplay = !gToggleDisplay;
  }
}
void fctEtape7() { // Fin de Charge détectée
  if (entry_FinCharge) { // OneShot
    digitalWrite(PIN_LED, LOW);   // <<< LED OFF à la fin
    DBGLN("Fin de Charge");
    lcdFinCharge();
    ssrOff();
    updateRuntime();
    lastRFID = "";
    gUID = "";
    LogEvent("STOP", "Fin de charge " + gQuitMode);
    clearUIDFromEEPROM();
    resetActivityTimer();
    entry_FinCharge = false;
  }
  if (isInactive(2000)) {
    state = ATTENTE_CARTE;
    entry_AttenteCarte = true;
  }
}
void fctEtape8() { // Mode Administration
  if (entry_ModeAdmin) { // OneShot
    DBGLN("Mode Admin");
    LogEvent("ADMIN", "Entrée en mode administrateur");
    lcdAdmin();
    entry_ModeAdmin = false;
  }
  // Commandes admin
  handleSerialCommands();
  resetActivityTimer();
  
}
