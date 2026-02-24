#include <Arduino.h>
#include <EEPROM.h>

#define EEPROM_ADDR_CURRENT_BAT  200
#define DEBUG 1   // Mets 0 pour désactiver
#if DEBUG
  #define DBG(x)    Serial.print(x)
  #define DBGLN(x)  Serial.println(x)
#else
  #define DBG(x)
  #define DBGLN(x)
#endif

// Prototype des Fonctions (obligatoire en PlatformIO)
void selectBattery(byte bat);
float loadFloat(int value);
void saveFloat(int address, float value);
void handleSerialCommands();


// --- PINS DIP SWITCH (LSB → MSB) ---
const byte dipPins[4] = {10, 11, 12, 13};

// --- MAXIMUM POSSIBLE ---
#define MAX_BAT 8

// --- PINS RELAIS ---
const byte relayPins[MAX_BAT] = {2, 3, 4, 5, 6, 7, 8, 9};
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// --- NOMBRE DE BATTERIES ACTIF ---
byte BAT_COUNT = 0;

// --- SEQUENCEUR ---
byte currentBat = 0;

// --- TEMPS ---
unsigned long chargeStart = 0;
unsigned long chargeDuration = 0;

// ------------------------------------------------------------
// LECTURE DIP SWITCH
// ------------------------------------------------------------
byte readDipValue() {
  byte b0 = !digitalRead(dipPins[0]); // inversion car INPUT_PULLUP
  byte b1 = !digitalRead(dipPins[1]);
  byte b2 = !digitalRead(dipPins[2]);
  byte b3 = !digitalRead(dipPins[3]); 
  return (b3 << 3) | (b2 << 2) | (b1 << 1) | b0;  // valeur 0..15
}

byte decodeBatteryCount(byte dipValue) {
  if (dipValue == 0) return 0;   // 000 = OFF
  if (dipValue >= 7) return 8;   // 111 = 8 batteries
  return dipValue + 1;           // 001→2, 010→3, ..., 110→7
}
// ------------------------------------------------------------
// SETUP
// ------------------------------------------------------------
void setup() {
  Serial.begin(9600);

  // DIP pins
  for (byte i = 0; i < 4; i++) {
    pinMode(dipPins[i], INPUT_PULLUP);
  }

  // Lire DIP
  byte dipValue = readDipValue();
  byte batValue = dipValue & 0b00000111;
  bool longCharge = (dipValue & 0b00001000) != 0;
  
  BAT_COUNT = decodeBatteryCount(batValue);

  if (BAT_COUNT == 0) {
    Serial.println("SYSTEME DESACTIVE (DIP = 000)");
    while (1) delay(1000);
  }
  
  Serial.print("Nombre de batteries detecte : ");
  Serial.println(BAT_COUNT);

  // Durée selon DIP B3
  chargeDuration = longCharge ? 60UL * 60UL * 1000UL : 90UL * 60UL * 1000UL;
  Serial.print("Durée de charge : ");
  Serial.print(chargeDuration / 60000UL);
  Serial.println(" minutes");

  Serial.print("DipValue:");
  Serial.println(dipValue);

  // Relais
  for (byte b = 0; b < BAT_COUNT; b++) {
    pinMode(relayPins[b], OUTPUT);
    digitalWrite(relayPins[b], RELAY_OFF);
  }

  // Reprise EEPROM
  byte saved = EEPROM.read(EEPROM_ADDR_CURRENT_BAT);
  if (saved < BAT_COUNT) currentBat = saved;
  else currentBat = 0;
  Serial.print("Reprise : Batterie B");
  Serial.println(currentBat);
  selectBattery(currentBat);
  chargeStart = millis();
}

// ------------------------------------------------------------
// LOOP PRINCIPALE
// ------------------------------------------------------------
void loop() {
  // Fin du délai ?
  if (millis() - chargeStart >= chargeDuration) {

    Serial.print(">>> Temps ecoule pour B");
    Serial.println(currentBat);

    // Éteindre tous les relais
    for (byte i = 0; i < BAT_COUNT; i++) {
      digitalWrite(relayPins[i], RELAY_OFF);
    }

    Serial.println(">>> Transition OFF 5 secondes");
    delay(5000);

    // Batterie suivante
    currentBat++;
    if (currentBat >= BAT_COUNT) currentBat = 0;

    EEPROM.update(EEPROM_ADDR_CURRENT_BAT, currentBat);

    Serial.print(">>> Passage a B");
    Serial.println(currentBat);

    selectBattery(currentBat);
    chargeStart = millis();
  }
  delay(200);
 }

// ------------------------------------------------------------
// RELAIS
// ------------------------------------------------------------
void selectBattery(byte b) {
  for (byte i = 0; i < BAT_COUNT; i++) {
    digitalWrite(relayPins[i], RELAY_OFF);
  }
  digitalWrite(relayPins[b], RELAY_ON);
}
// ------------------------------------------------------------
// EEPROM
// ------------------------------------------------------------
void saveFloat(int address, float value) {
  EEPROM.put(address, value);
}
float loadFloat(int address) {
  float value;
  EEPROM.get(address, value);
  return value;
}