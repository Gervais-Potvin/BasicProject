#include "hardware.h"
#include "config.h"

void initDIPSwitches() {
  // Initialise les pins des DIP Switches en INPUT_PULLUP
  pinMode(DIP1, INPUT_PULLUP);
  pinMode(DIP2, INPUT_PULLUP);
  pinMode(DIP3, INPUT_PULLUP);
  pinMode(DIP4, INPUT_PULLUP);
  DBGLN("DIP Switches initialisés en INPUT_PULLUP");
}
int readDIPSwitches() {
  // Lit l'état des 4 DIP Switches et retourne une valeur de 0 à 15
  int value = 0;
  value |= (digitalRead(DIP1) == LOW) ? 1 : 0;   // DIP1 = bit 0
  value |= (digitalRead(DIP2) == LOW) ? 2 : 0;   // DIP2 = bit 1
  value |= (digitalRead(DIP3) == LOW) ? 4 : 0;   // DIP3 = bit 2
  value |= (digitalRead(DIP4) == LOW) ? 8 : 0;   // DIP4 = bit 3
  return value;
}