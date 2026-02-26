#include <MFRC522.h>

#include "state_machine.h"  //Projet
#include "hardware.h" // Projet
#include "sdcard.h" // Projet
#include "rtc.h" // Projet
#include "rfid.h" // Projet
#include "lcd.h" // Projet
#include "config.h" // Projet
#include "sct013.h" // Projet
#include "ssr.h" // Projet
#include "eprom.h" // Projet
#include "serial.h" // Projet
#include "dipswitch.h" // Projet

extern unsigned long ulngSsrStartTime;   // moment où les SSR ont été activés

State state = ATTENTE_CARTE;
bool entry_AttenteCarte = false;
bool entry_ValidationCarte = false;
bool entry_CarteValide = false;
bool entry_CarteInvalide = false;
bool entry_AttenteConnexion = false;
bool entry_ChargeEnCours = false;
bool entry_FinCharge = false;
bool entry_ModeAdmin = false;
String gstrRFID = "";
String lastRFID = "";

extern LiquidCrystal_I2C lcd;

// Informations Globale sur la carte courante
bool gCardFound = false;
// Variables Globales de temps



bool isInactive(unsigned long timeoutMs);
bool isCharging();
String readLine();
String readSerialString();
void handleSerialCommands();
// SETUP
void setup() {

  initSerial();

  initDIPSwitches();
  
  pinMode(53, OUTPUT); // SS matériel du Mega
  pinMode(PIN_LED, OUTPUT);

  pinMode(PIN_AC_DETECT, INPUT);

  sdDeselectAll();    // Sécurise le bus SPI
  Wire.begin();
  SPI.begin();

  // Initialisation du RTC
  initRTC();

  rtcBatteryOK();

  // Initialisation du LCD
  InitLCD();

  // --- INITIALISATION RFID ---
  initRFID();

  // --- INITIALISATION SD ---
  initSD();

  Serial.println("=== BORNE AMICALE — Fonctions Sérielles réservé Administration ===");
  Serial.println(" admin    → Passer en mode Administrateur");
  Serial.println("----------------------------------------");

  getTimestamp(); // Initialisation
  LogEvent("BOOT", "Redémarrage du système");

  // --- Lecture EEPROM pour reprise apres perte de tension ou reset ---
  String resumeRFID = readUIDFromEEPROM();
  DBG("resumeRFID: ");
  DBGLN(resumeRFID);
  if (isValidRFID(resumeRFID)) {
    gstrRFID = resumeRFID;
    // Activer SSR pour permettre la détection de courant
    ssrOn();
    delay(5000);  // temps pour stabiliser le courant
    if (checkStartByCurrent()) {
      // Recharger les infos de la carte depuis la SD
      if (!findCard(gstrRFID)) {
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
  DBG(resumeRFID);
  DBG("]");
}
// =========================
// LOOP
// =========================
void loop() {
  // Gestion du LCD Rest Mode (indépendant)
  lcdIdleUpdate();
  // 1. Exécuter les actions de l’étape courante
  runStateActions();
  // 2. Gérer les transitions entre étapes
  handleTransitions();
}
