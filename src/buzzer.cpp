#include "buzzer.h"
#include "hardware.h" // Projet

void beep(int durationMs) {
  // Buzzer pendant un délai en millisecondes
  digitalWrite(PIN_BUZZER, HIGH);
  delay(durationMs);
  digitalWrite(PIN_BUZZER, LOW);
}

void initBuzzer() {
  // Initialisation du buzzer (si besoin)
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
}