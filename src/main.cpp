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
void loadCalibration(byte bat);
void saveCalibration(byte bat);
void calibrateTwoPoints(byte bat);
bool isChargeComplete(byte bat);
void selectBattery(byte bat);
void calibrateAllChannels();
float loadFloat(int value);
float readFilteredVoltage(byte bat);
void saveFloat(int address, float value);
void handleSerialCommands();


// --- CONFIGURATION DU PONT DIVISEUR ---
const float R1 = 20000.0;
const float R2 = 10000.0;

// --- PINS DIP SWITCH (LSB → MSB) ---
const byte dipPins[4] = {10, 11, 12, 13};

// --- MAXIMUM POSSIBLE ---
#define MAX_BAT 8

// --- PINS ANALOGIQUES ---
const byte analogPins[MAX_BAT] = {A0, A1, A2, A3, A4, A5, A6, A7};

// --- PINS RELAIS ---
const byte relayPins[MAX_BAT] = {2, 3, 4, 5, 6, 7, 8, 9};
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// --- FILTRAGE ---
const byte FILTER_SIZE = 20;
int  filterBuffer[MAX_BAT][FILTER_SIZE];
byte filterIndex[MAX_BAT] = {0};

// --- CALIBRATION ---
float gain[MAX_BAT];
float offset[MAX_BAT];

// --- NOMBRE DE BATTERIES ACTIF ---
byte BAT_COUNT = 0;

// --- SEQUENCEUR ---
byte currentBat = 0;

// --- TEMPS ---
unsigned long chargeStart = 0;
unsigned long chargeDuration = 0;
unsigned long lastVoltagePrint = 0;
const unsigned long VOLTAGE_PRINT_INTERVAL = 1UL * 10UL * 1000UL; // 1 minutes
unsigned long lastVoltageUpdate = 0;
const unsigned long VOLTAGE_UPDATE_INTERVAL = 500; // 1/2 seconde

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

  // Init buffers
  for (byte b = 0; b < BAT_COUNT; b++) {
    for (byte i = 0; i < FILTER_SIZE; i++) {
      filterBuffer[b][i] = 0;
    }
    gain[b]   = 1.0;
    offset[b] = 0.0;
  }

  // Pré-remplir le filtre avec des valeurs réelles
  for (byte b = 0; b < BAT_COUNT; b++) {
    for (byte i = 0; i < FILTER_SIZE; i++) {
        filterBuffer[b][i] = analogRead(analogPins[b]);
        delay(5);
    }
  }
  // Relais
  for (byte b = 0; b < BAT_COUNT; b++) {
    pinMode(relayPins[b], OUTPUT);
    digitalWrite(relayPins[b], RELAY_OFF);
  }

  // Charger calibrations EEPROM
  for (byte b = 0; b < BAT_COUNT; b++) {
    loadCalibration(b);
    if (isnan(gain[b]) || gain[b] == 0.0) gain[b] = 1.0;
    if (isnan(offset[b])) offset[b] = 0.0;
  }
  Serial.println("Commandes : CAL2 X  |  CAL2 ALL");

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
  handleSerialCommands();

  // Rafraîchir les tensions chaque seconde
  if (millis() - lastVoltageUpdate >= VOLTAGE_UPDATE_INTERVAL) {
    lastVoltageUpdate = millis();

    // Lire toutes les tensions pour mettre à jour les filtres
    for (byte b = 0; b < BAT_COUNT; b++) {
        readFilteredVoltage(b);  // met à jour le filtre
    }
  }

  if (millis() - lastVoltagePrint >= VOLTAGE_PRINT_INTERVAL) {
    // --- Affichage périodique des tensions ---
    // Forcer l'échelle 0–15 V
    Serial.println("MIN:11 ");
    Serial.println("MAX:14 ");
    lastVoltagePrint = millis();
    float delaiNext = (chargeDuration - (millis() - chargeStart)) / 60000.0; // Délai avant le passage à la batterie suivante
    for (byte b = 0; b < BAT_COUNT; b++) {
        float v = readFilteredVoltage(b);
        Serial.print("B");
        Serial.print(b);
        Serial.print(":");
        Serial.print(v, 2);
        Serial.println(" ");
    }
    Serial.print("B4:");
    Serial.print(delaiNext, 1);
    Serial.println();
  }
  

  // Fin du délai ?
  if (millis() - chargeStart >= chargeDuration) {

    Serial.print(">>> Temps ecoule pour B");
    Serial.println(currentBat);

    // Éteindre tous les relais
    for (byte i = 0; i < BAT_COUNT; i++) {
      digitalWrite(relayPins[i], RELAY_OFF);
    }

    Serial.println(">>> Transition OFF 20 secondes");
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
// MESURE FILTREE
// ------------------------------------------------------------
float readFilteredVoltage(byte index) {
  int raw = analogRead(analogPins[index]);
  filterBuffer[index][filterIndex[index]] = raw;
  filterIndex[index] = (filterIndex[index] + 1) % FILTER_SIZE;
  long sum = 0;
  for (byte i = 0; i < FILTER_SIZE; i++) sum += filterBuffer[index][i];
  float avgRaw = sum / (float)FILTER_SIZE;
  float voltageADC  = avgRaw * (5.0 / 1023.0);
  float voltageReal = voltageADC * ((R1 + R2) / R2);
  return gain[index] * voltageReal + offset[index];
}

// ------------------------------------------------------------
// MESURE FILTREE FRAICHE (CALIBRATION)
// ------------------------------------------------------------
float readFilteredVoltageFresh(byte index) {
  for (byte i = 0; i < FILTER_SIZE; i++) {
    filterBuffer[index][i] = analogRead(analogPins[index]);
    delay(5);
  }
  long sum = 0;
  for (byte i = 0; i < FILTER_SIZE; i++) sum += filterBuffer[index][i];
  float avgRaw = sum / (float)FILTER_SIZE;
  float voltageADC  = avgRaw * (5.0 / 1023.0);
  return voltageADC * ((R1 + R2) / R2);
}

// ------------------------------------------------------------
// DETECTION FIN DE CHARGE
// ------------------------------------------------------------
bool isChargeComplete(byte b) {
  static float lastV[MAX_BAT] = {0};
  static unsigned long stableTime[MAX_BAT] = {0};
  float v = readFilteredVoltage(b);
  if (v >= 14.40) return true;
  if (abs(v - lastV[b]) < 0.01) {
    if (millis() - stableTime[b] > 30000) return true;
  } else {
    stableTime[b] = millis();
  }
  lastV[b] = v;
  return false;
}

// ------------------------------------------------------------
// COMMANDES SERIE
// ------------------------------------------------------------
void handleSerialCommands() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd.startsWith("CAL2")) {
    int spaceIndex = cmd.indexOf(' ');
    if (spaceIndex > 0) {
      String arg = cmd.substring(spaceIndex + 1);
      arg.trim();
      if (arg == "ALL") {
        calibrateAllChannels();
      } else {
        int bat = arg.toInt();
        if (bat >= 0 && bat < BAT_COUNT) calibrateTwoPoints(bat);
        else Serial.println("Canal invalide.");
      }
    }
  }
}

// ------------------------------------------------------------
// CALIBRATION INDIVIDUELLE
// ------------------------------------------------------------
void calibrateTwoPoints(byte bat) {
  Serial.println("=== CALIBRATION CANAL ===");
  Serial.println("1) Entrez la valeur basse (ex: 5.00)");
  while (!Serial.available()) {}
  float Vlow = Serial.readStringUntil('\n').toFloat();
  float Mlow = readFilteredVoltageFresh(bat);
  Serial.println("2) Entrez la valeur haute (ex: 12.00)");
  while (!Serial.available()) {}
  float Vhigh = Serial.readStringUntil('\n').toFloat();
  float Mhigh = readFilteredVoltageFresh(bat);
  gain[bat]   = (Vhigh - Vlow) / (Mhigh - Mlow);
  offset[bat] = Vlow - gain[bat] * Mlow;
  saveCalibration(bat);
  Serial.println("Calibration OK.");
}

// ------------------------------------------------------------
// CALIBRATION GLOBALE
// ------------------------------------------------------------
void calibrateAllChannels() {
  Serial.println("=== CALIBRATION GLOBALE ===");
  Serial.println("1) Entrez la valeur basse");
  while (!Serial.available()) {}
  float Vlow = Serial.readStringUntil('\n').toFloat();
  float Mlow[MAX_BAT];
  for (byte b = 0; b < BAT_COUNT; b++) {
    Mlow[b] = readFilteredVoltageFresh(b);
  }
  Serial.println("2) Entrez la valeur haute");
  while (!Serial.available()) {}
  float Vhigh = Serial.readStringUntil('\n').toFloat();
  float Mhigh[MAX_BAT];
  for (byte b = 0; b < BAT_COUNT; b++) {
    Mhigh[b] = readFilteredVoltageFresh(b);
  }
  for (byte b = 0; b < BAT_COUNT; b++) {
    gain[b]   = (Vhigh - Vlow) / (Mhigh[b] - Mlow[b]);
    offset[b] = Vlow - gain[b] * Mlow[b];
    saveCalibration(b);
  }
  Serial.println("Calibration globale OK.");
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

void saveCalibration(byte bat) {
  int base = bat * 8;
  saveFloat(base,     gain[bat]);
  saveFloat(base + 4, offset[bat]);
}

void loadCalibration(byte bat) {
  int base = bat * 8;
  gain[bat]   = loadFloat(base);
  offset[bat] = loadFloat(base + 4);
}