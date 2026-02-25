#include <Arduino.h>
#include "state_machine.h"
#include "config.h"
#include "rfid.h"
#include "sdcard.h"
#include "lcd.h"
#include "rtc.h"
#include "sct013.h"
#include "hardware.h"
#include "admin.h"
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
  String uid = "";
  switch(state) {
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
      uid = readRFID();
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
  //float amps = readSCT();
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