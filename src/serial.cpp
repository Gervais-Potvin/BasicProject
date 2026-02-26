#include "serial.h"
#include "config.h" // Projet
#include "rtc.h"    // Projet
#include "sdcard.h" // Projet
#include "lcd.h"    // Projet

extern LiquidCrystal_I2C lcd;
extern unsigned long ulngLastActivity;

static String cmd = "";

String readSerialString()
{
  // Lecture de string provenant du SerialMonitor
  while (!Serial.available())
  {
  }
  return Serial.readStringUntil('\n');
}
void handleSerialCommands()
{
  // Fonction Administrateur Écoute du Serial Monitor pour le traitement des commandes Administrateur
  if (!Serial.available())
    return;
  ulngLastActivity = millis();
  cmd = Serial.readStringUntil('\n');
  cmd.trim();
  // --- Activation du mode admin ---
  if (cmd == "admin")
  {
    state = MODE_ADMIN;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("== MODE ADMIN ==");
    lcd.setCursor(0, 2);
    lcd.print("Console active");
    Serial.println("=== BORNE AMICALE — Fonctions réservées à l'Administration ===");
    Serial.println(" rtc      → Lire l'heure");
    Serial.println(" set      → Définir l'heure");
    Serial.println(" addcard  → Enregistrer une nouvelle carte RFID");
    Serial.println(" delcard  → Effacer une carte RFID du Systeme");
    Serial.println(" findcard → Trouver une carte RFID dans la liste");
    Serial.println(" exit     → Quitter le Mode Administrateur");
    Serial.println("---------------------------------------------------------------");
    return;
  }
  // --- Sortie du mode admin ---
  if (cmd == "exit")
  {
    state = ATTENTE_CARTE;
    entry_AttenteCarte = true;
    entry_ModeAdmin = true;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sortie Admin...");
    delay(1000);
    Serial.println("Retour au mode normal.");
    return;
  }
  // --- Commandes admin disponibles UNIQUEMENT en mode admin ---
  if (state == MODE_ADMIN)
  {
    if (cmd == "rtc")
      readRTC();
    else if (cmd == "set")
      setRTC();
    else if (cmd == "addcard")
      addCard();
    else if (cmd == "delcard")
      delCard();
    else if (cmd == "findcard")
    {
      if (findCard(""))
        Serial.println("Carte valide !");
      else
        Serial.println("Carte inconnue !");
    }
    else
      Serial.println("Commande inconnue en mode admin.");
    return;
  }
  // --- Si on n'est PAS en mode admin ---
  Serial.println("Commande non disponible hors mode admin.");
}
void initSerial()
{
  Serial.begin(9600);
  while (!Serial)
  {
  }
  Serial.println("=== BORNE AMICALE — Initialisation du Serial Monitor ===");
  Serial.println("Serial Monitor prêt.");
}