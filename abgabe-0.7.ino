/*
 * CatFeeder
 * Version: 0.7 (beta)
 * 
 * Führt den CatFeeder (Katzenfutterautomaten) aus.
 * 
 * Benötigte Hardware:
 * - ARDUINO UNO BOARD
 * - STEPPER MOTOR 2048 SCHRITTEN 
 *     --> 10 Slots (bei 9 Rationen gleich ca. 205 Steps pro Ration)
 * - PHOTO SENSOR / Fotozelle (Messung von Helligkeit)
 * - WEISSE LED
 * - RFID KIT VON FENDUINO
 * - DRUCK-KNOPF
 * - LC-DISPLAY VON FENDUINO (0x27, 16, 2)
 * - 220 Ohm Widerstand
 * 
 * Autor: Subi, 08.03.2021
 * Webseite: https://flsk.io/catfeeder
 * Dokumentation: https://flsk.io/catfeeder
 */


/*
 * START - Einbinden von Bibliotheken aller verbauten Kompontenten & Zusätzen
 */
#include <SPI.h> //RFID
#include <MFRC522.h> //RFID Board
#include <Stepper.h> //Stepper Motor
#include <Wire.h> //Display
#include <LiquidCrystal_I2C.h> //Display
#include <Time.h> //Timer


/*
 * START - Timer definieren
 */
time_t seconds = 0; //Sekunden gestellt auf Zero
long timeTag = 86400; //Ein Tag in Sekunden


/*
 * START - Display Definition
 * Definition des Displaytyps, hier zwei Zeilig mit 16 Zeichnenmöglichkeiten und unverlötet, 
 * sprich -->  HEX 0x27 ohne Lötstellenverlinkung
 */
LiquidCrystal_I2C lcd(0x27, 16, 2);

/*
 * START - Photonensensor
 */
int eingang = A0; // Definition des Eingangs für den Photonensensor
int fotoZelle = 0; // Sensorwert von Zero aus gestellt, sorgt für keinen anderen Startwert


/*
 * START - Schrittmotor
 */
// Initialisierung des Schrittmotors
int SPU = 2048; //Schritte pro Umdrehung, das der Motor unterstützt
Stepper StepMotor(SPU, 3,5,4,6); //Bezeichnung des Schrittmotors mit "StepMotor" und die eingesteckten Pins


/*
 * START - RFID
 */
//RFID PIN Steckerdefinition
#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);


/*
 * START - Start Variablen
 */
// Definition von der Anzahl an Rationen im feeder
int rationen = 9;

// Definieren von erhaltener max. Ration von einer Ration pro Tag
int erhalten = 0;

//Definition des Katzenchip-Codes
long checkCode = 2088700;

//Gemessene Definition des Tag-Werts in der Dämmerung am Morgen mit 266 (gemessen in Burgdorf)
int checkSensor = 266;


/*
 * START - VOID SETUP
 */
void setup() {
  //Baudrate 9600 für den Serial Monitor definiert
  Serial.begin(9600);

  // Timer in Sekunden
  //setTime(seconds);
  
  //RFID instanzieren und starten
  SPI.begin();
  mfrc522.PCD_Init();

  //Weisse LED über den Ausgang/Pin 2 ansteuern
  pinMode (2, OUTPUT); 
  
  //LCD Initialisierung inkl. Curser-Set und Displaybeleuchtung
  StepMotor.setSpeed(5); // Angabe der Geschwindigkeit in Umdrehungen pro Minute.

  //LC-Display
  lcd.init(); //LCD starten
  lcd.backlight(); //Beleuchtung einschalten
  lcd.setCursor(0, 0); //Position der ersten Zeile inkl. erstes Zeichen gesetzt
  lcd.print("Starte CatFeeder 0.7"); //Text Rationen übrig ausgeben
  lcd.setCursor(8, 1);// (8,1) Zweite Zeile und 8 für achtes -> "Mitte Display" Zeichen in der zweiten Zeile
  lcd.print("...");
  delay (6000); // für 5 Sekunden
}


/*
 * START - VOID LOOP
 */
void loop(){
  
  /*
 * LCD Setup und Iinitialnachricht definieren
 */
  lcd.setCursor(0, 0); //Position der ersten Zeile inkl. erstes Zeichen gesetzt
  lcd.print("Rationen uebrig:"); //Text Rationen übrig ausgeben 
  lcd.setCursor(8, 1);// (8,1) Zweite Zeile und 8 für achtes -> "Mitte Display" Zeichen in der zweiten Zeile
  lcd.print(rationen);

  
  //Sekunden zählen und abspeichern
  seconds = now();

  // Timer und Erhaltene Rationen nach einem Tag zurücksetzen
  if (seconds>timeTag){
    erhalten = 0;
    seconds = 0;
  }

  
  /*
   * Fotozelle auslesen und ausgeben
   */
  fotoZelle = analogRead(eingang); //Auslesen der Fotozelle am Analogeingang, wie oben definiert und deren Spannung in mW und Speicherung in die Variable
  
  /*
   * Console Ausgaben die für die Fehleranalyse hilfreich sind
   * Auskommentieren, falls notwendig.
   */
  //Serial.println("Rationen:");
  //Serial.println(rationen);
  //Serial.print("Die Kartennummer lautet mit check :");
  //Serial.println(code);
  //Serial.println(FotoZelle);
  //Serial.println(erhalten);
  //Serial.print("Foto Sensor: " ); //Serial Ausgabe der Fotozelle (also hier wollen wir die Dämmerung am Morgen bestimmen
  //Serial.println(fotoZelle); //Ausgabe am Serial-Monitor. Mit dem Befehl Serial.print wird der Sensorwert des Fotowiderstandes in Form einer Zahl zwischen 0 und 1023 an den serial monitor gesendet.
  //Serial.print("Foto Sensor: " ); //Ausgabe mit Text für die einfachere Identifizierung in der Konsole / Serial Monitor
  //Serial.println(fotoZelle); //Ausgabe am Serial-Monitor. Mit dem Befehl Serial.print wird der Sensorwert des Fotowiderstandes in Form einer Zahl zwischen 0 und 1023 an den serial monitor gesendet.


  /*
   * RFID Chipleser ausführen und in der Console ausgeben
   */
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
      return;
  }

  if ( ! mfrc522.PICC_ReadCardSerial()) {
      return;
  }
  long code=0; // Variable für die Kartenidentifikationscode auf Zero setzen, sodass sie immer wieder neu eingelesen werden kann
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    code=((code+mfrc522.uid.uidByte[i])*10); //ausgelesenen Code in human-read Form abspeichern
  }


  /*
   * Folgende Punkte werden nun überprüft, bevor es Futter gibt:
   * - Chip Code der Katze
   * - Ob es bereits hell ist (also nicht in der Nacht)
   * - Rationen nicht ausgegangen
   * - Ob am aktuellen Tag bereits eine Ration ausgegeben wurde und ob der Tag bereits durch ist
   */
  
  if (code == checkCode && fotoZelle > checkSensor && rationen > 0 && seconds<timeTag && erhalten == 0)
    { 
      // Status LED einschalten
      digitalWrite (2, HIGH);
      
      // Ration abziehen und auf dem Display darstellen
      rationen = rationen-1;
      StepMotor.step(210); // Der Motor macht 205 Steps, das ist die Anzahl Schritte für eine Ration (ca. 2048/10)
      delay (5000); // für 5 Sekunden
      // Status LED wieder aus
      digitalWrite (2, LOW);

      //ration erhalten aktualisieren:
      erhalten = 1;
      
    }
    
} // ENDE VOID LOOP
