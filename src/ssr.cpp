#include "ssr.h"

#include "hardware.h"

unsigned long ulngSsrStartTime = 0; // moment où les SSR ont été activés
unsigned long ulngSsrAccTime = 0;   // temps total accumulé en millisecondes
unsigned long ulngRuntime = 0;      // temps total formaté en minutes
unsigned long ulngRuntimeHr = 0;    // temps total formaté en heures
float fltRunCost = 0.0;             // coût total en dollars
float fltHourlyCost = 2.0;          // coût horaire de la charge en dollars
String strRuntime = "";             // temps total formaté en minutes
String strRuntimeHr = "";           // temps total formaté en heures
String strRunCost = "";             // coût total en dollars
bool blnSSRActive = false;

void initSSR()
{
  // Initialisation des pins de contrôle des SSR
  pinMode(PIN_SSR_A, OUTPUT);
  pinMode(PIN_SSR_B, OUTPUT);
  // Assure que les SSR sont éteints au démarrage
  digitalWrite(PIN_SSR_A, LOW);
  digitalWrite(PIN_SSR_B, LOW);
}
void ssrOn()
{
  // Activation simultanée des 2 SolidState Relays
  digitalWrite(PIN_SSR_A, HIGH);
  digitalWrite(PIN_SSR_B, HIGH);
  if (!blnSSRActive)
  {
    ulngSsrStartTime = millis();
    blnSSRActive = true;
  }
}
void ssrOff()
{
  // Désactivation simultanée des 2 SolidState Relays
  digitalWrite(PIN_SSR_A, LOW);
  digitalWrite(PIN_SSR_B, LOW);
  if (blnSSRActive)
  {
    ulngSsrAccTime += millis() - ulngSsrStartTime;
    blnSSRActive = false;
  }
}
void updateRuntime()
{
  // Mise à jour du temps de fonctionnement des SolidState Relays
  unsigned long elapsed = millis() - ulngSsrStartTime;
  ulngRuntime = elapsed / 60000UL; // minutes écoulées
  fltRunCost = (ulngRuntime / 60.0) * fltHourlyCost;
  ulngRuntimeHr = ulngRuntime / 60UL;

  strRuntime = String(ulngRuntime, 0);
  strRuntimeHr = String(ulngRuntimeHr, 2);
  strRunCost = String(fltRunCost, 2);
}
