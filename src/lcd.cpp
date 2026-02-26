#include "lcd.h"
#include "rtc.h"    // Projet
#include "config.h" // Projet

extern String strRunCost; // coût total en dollars
extern String strRuntime; // temps total formaté en minutes
extern String strPrenom;
extern String strNom;
extern String strVehicule;
extern String strTelephone;

unsigned long lastLCDUpdate = 0;
const unsigned long LCD_UPDATE_INTERVAL = 3000; // en ms
bool lcdIdle = false;
unsigned long ulngLCDIdleActivity = 0;
const unsigned long LCD_IDLE_TIMEOUT = 300000; // 300 sec avant repos ==> 300000 msec
const unsigned long LCD_IDLE_REFRESH = 2000;   // rafraîchissement minimal
unsigned long ulnglastLCDIdleRefresh = 0;
unsigned long ulnglastIdleBlink = 0;
const unsigned long IDLE_BLINK_INTERVAL = 30000; // 30 sec

LiquidCrystal_I2C lcd(0x27, 20, 4); // LCD 20x4 adresse 0x27. pour changer l'adresse voir A0-A1-A2 sur le controleur I2C

void lcdUpdateClock()
{
  lcd.setCursor(0, 0);
  lcd.print(getTimestamp());
}
void lcdAccueil()
{ // Étape initiale Attente de RFID
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(getTimestamp());
  lcd.setCursor(0, 1);
  lcd.print("Suivez instructions");
  lcd.setCursor(0, 2);
  lcd.print("en majuscules...");
  lcd.setCursor(0, 3);
  lcd.print("PASSEZ VOTRE CARTE");
}
void lcdValidationCarte()
{ // Validation de la carte RFID
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(getTimestamp());
  lcd.setCursor(0, 1);
  lcd.print("Validation Carte");
  lcd.setCursor(0, 2);
  lcd.print("UID:");
  lcd.print(strRFID);
  lcd.setCursor(0, 3);
  lcd.print("Veuillez patienter");
}
void lcdCarteValide()
{ // Carte Valide
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Carte Valide");
  lcd.setCursor(0, 1);
  lcd.print(strPrenom + " " + strNom);
  lcd.setCursor(0, 2);
  lcd.print("Tel:" + strTelephone);
  lcd.setCursor(0, 3);
  lcd.print("BRANCHEZ VEHICULE");
}
void lcdCarteInvalide()
{ // Carte Invalide
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Carte Invalide");
  lcd.setCursor(0, 1);
  lcd.print("Acces Interdit");
  lcd.setCursor(0, 2);
  lcd.print("CONTACTEZ GERVAIS");
  lcd.setCursor(0, 3);
  lcd.print("AU 418-818-7818");
}
void lcdAttenteConnexion()
{ // Attente de Branchement du Véhicule
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(getTimestamp());
  lcd.setCursor(0, 1);
  lcd.print(strPrenom + " " + strNom);
  lcd.setCursor(0, 2);
  lcd.print("5 MIN. POUR");
  lcd.setCursor(0, 3);
  lcd.print("BRANCHER VEHICULE");
}
void lcdChargeEnCours()
{ // Charge en Cours
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(getTimestamp());
  // Ligne 1,2,3 mise à jour dynamique dans fctEtape6()
}
void lcdFinCharge()
{ // Fin de Charge détectée
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(getTimestamp());
  lcd.setCursor(0, 1);
  lcd.print("Fin de la charge");
  lcd.setCursor(0, 2);
  lcd.print("Cout:$");
  lcd.print(strRunCost);
  lcd.setCursor(0, 3);
  lcd.print("Temps:");
  lcd.print(strRuntime);
  lcd.print("m");
}
void lcdAdmin()
{ // Mode Administration
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(getTimestamp());
  lcd.setCursor(0, 1);
  lcd.print("MODE ADMIN");
  lcd.setCursor(0, 2);
  lcd.print("Console active");
  lcd.setCursor(0, 3);
  lcd.print("En attente cmd");
}
void lcdClearLine(uint8_t line)
{
  lcd.setCursor(0, line);
  lcd.print("                    "); // 20 espaces pour un LCD 20x4
}
void lcdIdleUpdate()
{
  // --- 1. Entrée en repos ---
  if (!lcdIdle && (millis() - ulngLCDIdleActivity > LCD_IDLE_TIMEOUT))
  {
    lcdIdle = true;
    lcd.clear();
    lcd.noBacklight(); // <<< extinction rétroéclairage
    ulnglastLCDIdleRefresh = millis();
    return;
  }
  // --- 2. Mode repos actif ---
  if (lcdIdle)
  {
    lcd.noBacklight(); // <<< rétroéclairage toujours OFF en repos
    // Animation anti burn-in toutes les 10 secondes
    if (millis() - ulnglastLCDIdleRefresh > 10000)
    {
      static int pos = 0;
      static int direction = 1;
      lcd.backlight(); // <<< allume juste pour afficher l’animation
      lcd.clear();
      // Message qui se déplace verticalement
      lcd.setCursor(0, pos);
      lcd.print("Pour sortir du Repos");
      lcd.setCursor(0, pos + 1);
      lcd.print("PASSEZ UNE CARTE");
      // Mouvement vertical
      pos += direction;
      if (pos >= 2)
        direction = -1;
      if (pos <= 0)
        direction = 1;
      delay(2000);       // <<< visible brièvement
      lcd.noBacklight(); // <<< retourne en extinction
      ulnglastLCDIdleRefresh = millis();
    }
    return;
  }
}
void InitLCD()
{
  lcdIdle = false;
  lcdIdle = false;
  ulngLCDIdleActivity = millis();
  ulnglastLCDIdleRefresh = millis();
  lcd.init();
  lcd.backlight();
  lcdAccueil();
}