#pragma once
#include <Arduino.h>

#ifdef DEBUG
  #define DBG(x) Serial.print(x)
  #define DBGLN(x) Serial.println(x)
#else
  #define DBG(x)
  #define DBGLN(x)
#endif

// États définis dans la Machine
enum State {
    ATTENTE_CARTE,
    VALIDATION_CARTE,
    CARTE_VALIDE,
    CARTE_INVALIDE,
    ATTENTE_CONNEXION,
    CHARGE_EN_COURS,
    FIN_CHARGE,
    MODE_ADMIN
};

// Variable d'état globale
extern State state;

// Flags d’entrée
extern bool entry_AttenteCarte;
extern bool entry_ValidationCarte;
extern bool entry_CarteValide;
extern bool entry_CarteInvalide;
extern bool entry_AttenteConnexion;
extern bool entry_ChargeEnCours;
extern bool entry_FinCharge;
extern bool entry_ModeAdmin;

// UID
extern String strRFID; // UID de la carte en cours de validation
extern String strLastRFID; // Stocke le dernier RFID lu pour éviter les lectures multiples
extern const unsigned long LCD_UPDATE_INTERVAL; // en ms

// Autres variables globales


