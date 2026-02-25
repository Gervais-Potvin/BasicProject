#pragma once
#include <Arduino.h>

// Enum des états
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
extern String gUID;
extern String lastRFID;

// Autres variables globales (on les ajoutera au besoin)