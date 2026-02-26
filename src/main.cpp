#include <MFRC522.h>

#include "state_machine.h" //Projet
#include "hardware.h"      
#include "sdcard.h"        // Projet
#include "rtc.h"           // Projet
#include "rfid.h"          // Projet
#include "lcd.h"           // Projet
#include "config.h"        // Projet
#include "sct013.h"        // Projet
#include "ssr.h"           // Projet
#include "eprom.h"         // Projet
#include "serial.h"        // Projet
#include "dipswitch.h"     // Projet

extern unsigned long ulngSsrStartTime; // moment où les SSR ont été activés
extern LiquidCrystal_I2C lcd;

State state = ATTENTE_CARTE;
bool entry_AttenteCarte = false;
bool entry_ValidationCarte = false;
bool entry_CarteValide = false;
bool entry_CarteInvalide = false;
bool entry_AttenteConnexion = false;
bool entry_ChargeEnCours = false;
bool entry_FinCharge = false;
bool entry_ModeAdmin = false;

// SETUP
void setup()
{

  initSerial(); // Initialisation du Serial Monitor

  initDIPSwitches(); // Initialisation des DIP Switches

  pinMode(PIN_LED, OUTPUT); // LED témoin d'activité

  pinMode(53, OUTPUT);      // SS matériel du Mega
  //digitalWrite(53, HIGH); // Désactive le SS matériel pour éviter les conflits SPI

  //pinMode(SD_CS, OUTPUT); // CS de la carte SD
  //digitalWrite(SD_CS, HIGH); // Désactive la carte SD au démarrage
/*
  pinMode(RFID_SS, OUTPUT); // CS du lecteur RFID
  digitalWrite(RFID_SS, HIGH); // Désactive le lecteur RFID au démarrage

  Wire.begin(); // Initialisation du bus I2C pour le LCD
  initLCD(); // Initialisation du LCD
  initRTC(); // Initialisation du RTC
  rtcBatteryOK(); // Vérification de la batterie du RTC

  SPI.begin(); // Initialisation du bus SPI pour les capteurs de courant

  initSD(); // Initialisation de la carte SD et création du fichier de log si nécessaire
  digitalWrite(SD_CS, HIGH); //

  initRFID(); // Initialisation du lecteur RFID
  digitalWrite(RFID_SS, HIGH); // Désactive le lecteur RFID au démarrage

  Serial.println("=== BORNE AMICALE — Fonctions Sérielles réservé Administration ===");
  Serial.println(" admin    → Passer en mode Administrateur");
  Serial.println("----------------------------------------");

  getTimestamp(); // Initialisation
  LogEvent("BOOT", "Redémarrage du système");

  // --- Lecture EEPROM pour reprise apres perte de tension ou reset ---
  String resumeRFID = readUIDFromEEPROM();
  DBG("resumeRFID: ");
  DBGLN(resumeRFID);

  if (isValidRFID(resumeRFID))
  {
    strRFID = resumeRFID;
    // Activer SSR pour permettre la détection de courant
    ssrOn();
    delay(5000); // temps pour stabiliser le courant
    if (checkStartByCurrent())
    {
      // Recharger les infos de la carte depuis la SD
      if (!findCard(strRFID))
      {
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
    else
    {
      ssrOff();
      state = ATTENTE_CONNEXION;
      entry_AttenteConnexion = true;
      Serial.println("Reprise UID valide → attente connexion.");
    }
  }
  else
  {
    clearUIDFromEEPROM();
    state = ATTENTE_CARTE;
    entry_AttenteCarte = true;
    Serial.println("EEPROM invalide → nettoyage.");
  }
  
  DBG("Mode Admin");
  DBG("EEPROM UID au boot = [");
  DBG(resumeRFID);
  DBG("]");
  */
}

// LOOP
void loop()
{
  // Gestion du LCD Rest Mode (indépendant)
  lcdIdleUpdate();
  // 1. Exécuter les actions de l’étape courante
  runStateActions();
  // 2. Gérer les transitions entre étapes
  handleTransitions();
}
