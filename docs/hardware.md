# Connexions et Câblage — Arduino Nano

## Microcontrôleur
- Carte : Arduino Nano (ATmega328P)
- Alimentation : 5V USB ou Vin 7–12V

## Notes
- Ajouter ici les connexions spécifiques
- Ajouter les schémas ou photos si nécessaire
// =========================
## COMMENTAIRES
// =========================
## Version Fonctionnelle avec le matériel suivant:
// RTC DS3221
// RFID RC522
// Lecteur microSD avec Carte32 GBytes
// LCD 20 x 4
// Buzzer
// SSRA et SSRB
// Led vert
// SCT013-30
// Essais.docx : Tests effectués avec succès. Nombre de tests total: 11
// Date de la version : 2026/02/12 05:11 AM

## Fonctions Supplémentaires Testées:
// addCard : Scan un RFID et demande des questions pertinentes. Enregistrement dans Cardlist.csv sur la carte SD.
// delCard : Scan un RFID et efface l'enregistrement dans Cardlist.csv sur la carte SD.
// findCard("") : Scan un RFID et trouve l'enregistrement dans Cardlist.csv sur la carte SD, mets les informations en mémoire.
// findCard("E6:DC:70:05") : trouve l'enregistrement correspondant au UID dans Cardlist.csv sur la carte SD, mets les informations en mémoire.
// pour chaque Appel d'une sslecture RFID, on beep 200 msec.
// =========================
## CONNEXIONS
// =========================
## Détails des connexions Mega 2560
// Mega Section Power Pin 3.3V ----- RC522 Pin 3.3
// Mega Pin 50 ----- MISO RC522 ----- R-1K-Ohms ----- MISO microSD
// Mega Pin 51 ----- MOSI RC522 ----- MOSI microSD
// Mega Pin 52 ----- SCK RC522 ----- SCK microSD
// Mega Pin 20(SDA) ----- SDA RTC ----- SDA LCD
// Mega Pin 21(SCL) ----- SCL RTC ----- SCL LCD 
// Mega Pin 3 ----- RST RC522
// Mega Pin 4 ----- SDA RC522
// Mega Pin 5 ----- +(Buzzer)- ----- Gnd
// Mega Pin 6 ----- R-330-Ohms ----- +(Led Vert)- ----- Gnd
// Mega Pin 10 ----- CD microSD
// Mega Pin 7 ----- R-330-Ohms ----- Pin 3 Input SSR50-DA Phase A
// Mega Pin 12 ----- R-330-Ohms ----- Pin 3 Input SSR50-DA Phase B
// Mega Pin A0 ----- SCT013-30 Fil 1 (pas vraiment d'importance on mesure du AC)
// Mega Section Power Pin GND ----- +(Buzzer)- ----- Gnd

## Connexions supplémentaire RTC
// RTC Pin VCC ----- 5VCC Proto
// RTC Pin GND ----- GND Proto

## Connexions supplémentaire LCD
// LCD Pin VCC ----- 5VCC Proto
// LCD Pin GND ----- GND Proto

## Connexions supplémentaires Module SD
// microSD Pin VCC ----- 5VCC Proto
// microSD Pin GND ----- GND Proto

## Connexions Modules SSR50-DA Phase A/B
// Pin 4 Input SSR50-DA Phase A ----- GND
// Pin 4 Input SSR50-DA Phase B ----- GND
// Pin 1 Output SSR50-DA Phase A ----- L1 Panneau Distribution
// Pin 1 Output SSR50-DA Phase B ----- L2 Panneau Distribution
// Pin 2 Output SSR50-DA Phase A ----- L1 vers Alimentation Borne
// Pin 2 Output SSR50-DA Phase B ----- L2 vers Alimentation Borne

## Connexions Module SCT013-30
// SCT013-30 Autre Fil----- GND
