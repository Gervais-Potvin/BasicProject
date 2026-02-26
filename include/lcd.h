#pragma once
#include <LiquidCrystal_I2C.h>

void lcdAccueil();
void initLCD();
void lcdValidationCarte();
void lcdCarteValide();
void lcdCarteInvalide();
void lcdAttenteConnexion();
void lcdChargeEnCours();
void lcdFinCharge();
void lcdAdmin();
void lcdClearLine(uint8_t line);
void lcdIdleUpdate();
void lcdUpdateClock();