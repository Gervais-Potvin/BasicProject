#pragma once
#include <Arduino.h>

// Ajoute une carte dans le fichier SD
void addCard();
// Supprime une carte du fichier SD
void delCard();
// Vérifie si une carte existe dans le fichier SD
bool findCard(String uidParam);
// Selection du Module SD
void selectSD();
void initSD();
void sdDeselectAll();

void LogEvent(String type, String message);

void Log2SD(String timestamp, String uid, String info1, String info2, String info3);
bool checkSD();