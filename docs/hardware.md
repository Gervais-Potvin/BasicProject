# Connexions et Câblage — Arduino Nano

## Microcontrôleur
- Carte : Arduino Nano (ATmega328P)
- Alimentation : 5V USB ou Vin 7–12V

## Notes
- Ajouter ici les connexions spécifiques
- Ajouter les schémas ou photos si nécessaire
## Connexions
- Lecture des Voltages Batterie 0 à 7
- (Batt 1 +) _____ R1-0-20k ____ R2-0-10k _____ GND 
-                            |____ Nano Pin AO   
- (Batt 2 +) _____ R1-1-20k ____ R2-1-10k _____ GND 
-                            |____ Nano Pin A1   
- (Batt 3 +) _____ R1-2-20k ____ R2-2-10k _____ GND 
-                            |____ Nano Pin A2   
- (Batt 4 +) _____ R1-3-20k ____ R2-3-10k _____ GND 
-                            |____ Nano Pin A3   
- (Batt 5 +) _____ R1-4-20k ____ R2-4-10k _____ GND 
-                            |____ Nano Pin A4   
- (Batt 6 +) _____ R1-5-20k ____ R2-5-10k _____ GND 
-                            |____ Nano Pin A5   
- (Batt 7 +) _____ R1-6-20k ____ R2-6-10k _____ GND 
-                            |____ Nano Pin A6   
- (Batt 8 +) _____ R1-7-20k ____ R2-7-10k _____ GND 
-                            |____ Nano Pin A7 

## Connexion DipSwitch en INPUT_PULLUP
- Nano Pin D10 _____ DIP1 _____GND
- Nano Pin D11 _____ DIP2 _____GND
- Nano Pin D12 _____ DIP3 _____GND
- Nano Pin D13 _____ DIP4 _____GND

##  Configuration Nombre de Batterie via Dipswitch
- Dip1   Dip2    Dip2    Description
- OFF    OFF     OFF     Système Hors Fonction
- ON     OFF     OFF     2 Batteries
- OFF    ON      OFF     3 Batteries
- ON     ON      OFF     4 Batteries
- OFF    OFF     ON      5 Batteries
- ON     OFF     ON      6 Batteries
- OFF    ON      ON     7 Batteries
- ON     ON      ON     8 Batteries

## Configuration Mode de Fonctionnement via Dipswitch
- Dip4   Description
- OFF    Système avec Sélection Tension
- ON     Mode Mixte 

## Connexion Sorties Relais pour Chaque Batterie 
- On utilise 2 Modules de 4 sorties
- Module 1 utilisé pour les Batteries 1 à 4
- Nano Pin D2 _____ IN1 Module1 Controlera le Relais K1 de la Batterie 1
- Nano Pin D3 _____ IN2 Module1 Controlera le Relais K2 de la Batterie 2
- Nano Pin D4 _____ IN3 Module1 Controlera le Relais K3 de la Batterie 3
- Nano Pin D5 _____ IN4 Module1 Controlera le Relais K4 de la Batterie 4
- Supply Externe 5VCC _____ VCC Module1
- Supply Externe GND _____ GND Module1
- Relais K1 Module1 borne NO ______ + Batterie 1
- Relais K2 Module1 borne NO ______ + Batterie 2
- Relais K3 Module1 borne NO ______ + Batterie 3
- Relais K4 Module1 borne NO ______ + Batterie 4
- Relais K1 Module1 borne Commun ______ + Chargeur
- Relais K2 Module1 borne Commun ______ + Chargeur
- Relais K3 Module1 borne Commun ______ + Chargeur
- Relais K4 Module1 Module1 borne Commun ______ + Chargeur
- Module 2 utilisé pour les Batteries 5 à 8
- Nano Pin D6 _____ IN1 Module2 Controlera le Relais K1 de la Batterie 5
- Nano Pin D7 _____ IN2 Module2 Controlera le Relais K2 de la Batterie 6
- Nano Pin D8 _____ IN3 Module2 Controlera le Relais K3 de la Batterie 7
- Nano Pin D9 _____ IN4 Module2 Controlera le Relais K4 de la Batterie 8
- Supply Externe 5VCC _____ VCC Module2
- Supply Externe GND _____ GND Module2
- Relais K1 Module2 borne NO ______ + Batterie 5
- Relais K2 Module2 borne NO ______ + Batterie 6
- Relais K3 Module2 borne NO ______ + Batterie 7
- Relais K4 Module2 borne NO ______ + Batterie 8
- Relais K1 Module2 borne Commun ______ + Chargeur
- Relais K2 Module2 borne Commun ______ + Chargeur
- Relais K3 Module2 borne Commun ______ + Chargeur
- Relais K4 Module2 borne Commun ______ + Chargeur