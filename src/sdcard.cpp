#include "sdcard.h"
#include <SD.h>

#include "rfid.h" // Projet
#include "hardware.h" // Projet
#include "serial.h" // Projet
#include "config.h" // Projet
#include "rtc.h" // Projet

extern String strTimeStamp;
extern String strYear;

String strPrenom = "";
String strNom = "";
String strVehicule = "";
String strTelephone = "";
bool blnCardFound = false;

// Ajoute une carte dans le fichier SD
void addCard() {
  // Fonction Administrateur Ajout des informations relatives à une carte RFID sur la carte SD via Serial Monitor
  String uid = readRFID();
  // --- 2. DEMANDE PRÉNOM ---
  Serial.print("Prénom : ");
  String prenom = readSerialString();
  // --- 3. DEMANDE NOM ---
  Serial.print("Nom : ");
  String nom = readSerialString();
  // --- 4. DEMANDE VÉHICULE ---
  Serial.print("Véhicule : ");
  String vehicule = readSerialString();
  // --- 5. DEMANDE TELEPHONE ---
  Serial.print("Téléphone : ");
  String telephone = readSerialString();
  // --- 5. ENREGISTREMENT SUR SD ---
  selectSD();
  if (!SD.begin(SD_CS)) {
    Serial.println("Erreur SD");
    sdDeselectAll();
    return;
  }
  String filename = "Card" + strYear + ".csv";
  File f = SD.open(filename, FILE_WRITE);
  if (!f) {
    Serial.print("Impossible d'ouvrir ");
    Serial.println(filename);
    sdDeselectAll();
    return;
  }
  // Construction de la ligne CSV
  String line = "";
  // --- Ajout du Timestamp ---
  line += getTimestamp();
  line += ",";
  line += uid;
  line += ",";
  line += prenom;
  line += ",";
  line += nom;
  line += ",";
  line += vehicule;
  line += ",";
  line += telephone;
  f.println(line);
  f.close();
  sdDeselectAll();
  Serial.println("Carte ajoutée avec succès !");
  LogEvent("ADMIN", "Carte Ajouté" + uid);
}

void delCard() {
  // Fonction Administrateur Effacement des informations relatives à une carte RFID de la carte SD via Serial Monitor
  String uid = readRFID();
  // --- 2. OUVERTURE SD ---
  selectSD();
  if (!SD.begin(SD_CS)) {
    Serial.println("Erreur SD");
    sdDeselectAll();
    return;
  }
  String filename = "Card" + strYear + ".csv";
  File original = SD.open(filename, FILE_READ);
  if (!original) {
    Serial.print(filename);
    Serial.println(".csv introuvable");
    sdDeselectAll();
    return;
  }
  File temp = SD.open("temp.csv", FILE_WRITE);
  if (!temp) {
    Serial.println("Impossible de créer temp.csv");
    original.close();
    sdDeselectAll();
    return;
  }
  // --- 3. COPIE DES LIGNES SAUF CELLE À SUPPRIMER ---
  bool found = false;
  while (original.available()) {
    String line = original.readStringUntil('\n');
    // Si la ligne contient l’UID → on la saute
    if (line.indexOf(uid) != -1) {
      found = true;
      continue;
    }
    temp.println(line);
  }
  original.close();
  temp.close();
  // --- 4. REMPLACEMENT DU FICHIER ---
  SD.remove(filename);      // On supprime l'ancien
  File newfile = SD.open(filename, FILE_WRITE);
  File temp2 = SD.open("temp.csv", FILE_READ);
  while (temp2.available()) {
    newfile.write(temp2.read());
  }
  newfile.close();
  temp2.close();
  SD.remove("temp.csv");          // On supprime le fichier temporaire
  sdDeselectAll();
  // --- 5. MESSAGE FINAL ---
  if (found) {
    Serial.println("Carte supprimée avec succès !");
  } else {
    Serial.println("UID introuvable dans Cardlist.csv");
  }
  LogEvent("ADMIN", "Carte Supprimée " + uid);
}

bool findCard(String uidParam) {
  // Recherche de la présence des informations d'un UID d'une carte RFID sur la carte SD
  blnCardFound = false;
  strRFID = "";
  strPrenom = "";
  strNom = "";
  strVehicule = "";
  strTelephone = "";
  String uid = uidParam;
  // --- 1. SI UID VIDE → DEMANDER UNE CARTE ---
  if (uid.length() == 0) {
    uid = readRFID();
  }
  selectSD();
  if (!SD.begin(SD_CS)) {
    Serial.println("Erreur SD");
    sdDeselectAll();
    return false;
  }
  String filename = "Card" + strYear + ".csv";
  File f = SD.open(filename, FILE_READ);
  if (!f) {
    Serial.print(filename);
    Serial.println(" introuvable");
    sdDeselectAll();
    return false;
  }
  // --- 3. PARCOURS DU FICHIER ---
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    // Vérifie si la ligne contient l'UID
    if (line.indexOf(uid) != -1) {
      // Format :
      // date,UID,prenom,nom,vehicule,telephone
      int p1 = line.indexOf(',');               // fin date
      int p2 = line.indexOf(',', p1 + 1);       // fin UID
      int p3 = line.indexOf(',', p2 + 1);       // fin prénom
      int p4 = line.indexOf(',', p3 + 1);       // fin vehicule
      int p5 = line.indexOf(',', p4 + 1);       // fin telephone
      if (p1 > 0 && p2 > 0 && p3 > 0 && p4 > 0 && p5 > 0) {
        strRFID      = line.substring(p1 + 1, p2);
        strPrenom   = line.substring(p2 + 1, p3);
        strNom      = line.substring(p3 + 1, p4);
        strVehicule = line.substring(p4 + 1, p5);
        strTelephone = line.substring(p5 + 1);
        strRFID.trim();
        strPrenom.trim();
        strNom.trim();
        strVehicule.trim();
        strTelephone.trim();
        blnCardFound = true;
      }
      break;
    }
  }
  f.close();
  sdDeselectAll();
  LogEvent("OPERATION", "Recherche Carte " + uid);
  // --- 4. RETOURNE LE RÉSULTAT ---
  return blnCardFound;
}

void selectSD() {
  // Sélection de SD et Désélection du RFID
  digitalWrite(RFID_SS, HIGH); // RFID off
  digitalWrite(SD_CS, LOW);    // SD on
}
void sdDeselectAll() {
  // Désélection de SD et RFID
  digitalWrite(SD_CS, HIGH);
  digitalWrite(RFID_SS, HIGH);
}

void LogEvent(String type, String message) {
  // Sélection du module SD
  selectSD();
  if (!SD.begin(SD_CS)) {
    Serial.println("SD ERROR: Impossible d'initialiser la carte SD");
    sdDeselectAll();
    return;
  }
  // Nom du fichier basé sur l'année courante
  String filename = "Log" + strYear + ".csv";
  File f = SD.open(filename, FILE_WRITE);
  if (!f) {
    Serial.print("SD ERROR: Impossible d'ouvrir ");
    Serial.println(filename);
    sdDeselectAll();
    return;
  }
  // Construction de la ligne CSV
  String line = "";
  line += getTimestamp(); line += ",";
  line += type;          line += ",";
  line += strRFID;          line += ",";
  line += strPrenom;       line += ",";
  line += strNom;          line += ",";
  line += message;
  f.println(line);
  f.close();
  sdDeselectAll();
  Serial.print("LOG EVENT → ");
  Serial.println(line);
}
void Log2SD(String timestamp, String uid, String info1, String info2, String info3) {
  // Sélection du module SD
  selectSD();
  // Vérifie que la SD est prête
  if (!SD.begin(SD_CS)) {
    Serial.println("SD ERROR: Impossible d'initialiser la carte SD");
    sdDeselectAll();
    return;
  }
  // Nom du fichier basé sur l'année courante
  String filename = "Log" + strYear + ".csv";
  // Ouvre le fichier en mode append
  File f = SD.open(filename, FILE_WRITE);
  if (!f) {
    Serial.print("SD ERROR: Impossible d'ouvrir ");
    Serial.println(filename);
    sdDeselectAll();
    return;
  }
  // Construction de la ligne CSV
  String line = "";
  line += timestamp; 
  line += ",";
  line += uid;       
  line += ",";
  line += info1;    
  line += ",";
  line += info2;       
  line += ",";
  line += info3;
  // Écriture
  f.println(line);
  f.close();
  // Libère le bus SPI
  sdDeselectAll();
  Serial.print("SD LOG OK → ");
  Serial.println(line);
}
void initSD() {
  
  pinMode(SD_CS, OUTPUT);
  // Initialisation de la carte SD
  selectSD();
  if (!SD.begin(SD_CS)) {
    Serial.println("SD ERROR: Impossible d'initialiser la carte SD");
    sdDeselectAll();
    return;
  }
  Serial.println("SD OK");
  sdDeselectAll();
}
bool checkSD() {
  // Validation du fonctionnement de base de la carte SD
  selectSD();
  if (!SD.begin(SD_CS)) {
    Serial.println("SD FAIL");
    return false;
  }
  File f = SD.open("log.txt", FILE_WRITE);
  if (f) {
    f.println("Test SD OK");
    f.close();
    Serial.println("Écriture SD OK");
  } else {
    Serial.println("Impossible d'ouvrir log.txt");
    return false;
  }
  sdDeselectAll();
  return true;
}
