#include "sct013.h"
#include "config.h" // Projet

extern unsigned long ulngLastActivity;
extern bool blnSSRActive;

float currentAmps = 0.0;      // courant estimé
float amps_idle = 12.0; // Charge lue au Primaire de la borne pas de véhicule connecté
float amps_load = 16.0; // Charge lue au Primaire de la borne véhicule connecté
float currentStartThreshold = 14.5; // seuil en ampères pour dire "charge active"
float currentStopThreshold = 13.0; // seuil en ampères pour dire "charge inactive"

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
bool checkStopByCurrent() {
  if (!blnSSRActive) return false;   // SSR OFF → pas de charge en cours
  float amps = readSCT();
  // Détection fin de charge
  if (amps < currentStopThreshold) {
    Serial.print("Fin de charge détectée : ");
    Serial.print(amps);
    Serial.println(" A");
    ulngLastActivity = millis();
    return true;
  }
  return false;
}
bool checkStartByCurrent() {
  if (!blnSSRActive) return false;   // SSR OFF → pas de charge possible
  float amps = readSCT();
  // Détection début de charge
  if (amps > currentStartThreshold) {
    Serial.print("Début de charge détecté : ");
    Serial.print(amps);
    Serial.println(" A");
    ulngLastActivity = millis();
    return true;
  }
  return false;
}
bool isCharging() {  // True si on a le courant lu à l'entrée de la borne dépasse un seul minimum 
  if (!blnSSRActive) return false;
  float amps = readSCT();
  return amps > currentStartThreshold;
}
