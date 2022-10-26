/* This code works with 4x4 Keypad Matrix, LCD ic, IRF510N transistor and a push button
   It's a lock where the transistor drives a solenoid lock you can open either with correct code
   or by the push button from inside
   The code can be changed directly by the keypad and doesn't require uploading code again
   Code is stored in EEPROM
   Refer to www.surtrtech.com for more details
*/

/////////////////////////////////////////////////////////////////
//                  Arduino RFID Tutorial               v1.02  //
//       Get the latest version of the code here:              //
//         http://educ8s.tv/arduino-rfid-tutorial/             //
/////////////////////////////////////////////////////////////////


/*
  Arduino RFID Access Control

  Security !

  To keep it simple we are going to use Tag's Unique IDs
  as only method of Authenticity. It's simple and not hacker proof.
  If you need security, don't use it unless you modify the code

  Copyright (C) 2015 Omer Siar Baysal

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/

#include <MFRC522.h>
#include <SPI.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <IRremote.h>

//button setup
#define button 32

//led setup
#define LED_ON HIGH
#define LED_OFF LOW
#define ledR 38
#define ledG 36
#define ledB 34

//entry code
long int key = 9668331;
int keyEntry;

//lcd setup
LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

//ir remote setup
#define irReceiverPin 13 //the SIG of receiver module attach to pin7 
IRrecv irrecv(irReceiverPin); //Creates a variable of type IRrecv
decode_results results;
char irData;

//servo setup
#define serv 11
Servo s;
#define lockedPos 60
#define openPos 180
bool isLocked = LOW;

//keypad setup
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

char keypressed;                 //Where the keys are stored it changes very often
char code[] = {'9', '6', '6', '8'}; //The default code, you can change it or make it a 'n' digits one

short a = 0, i = 0, j = 0; //Variables used later

byte rowPins[ROWS] = {9, 8, 7, 6}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {10, 4, 3, 2}; //connect to the column pinouts of the keypad
//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);



#define kStart 0xFF22DD
#define k0 0xFF6897
#define k1 0xFF30CF
#define k2 0xFF18E7
#define k3 0xFF7A85
#define k4 0xFF10EF
#define k5 0xFF38C7
#define k6 0xFF5AA5
#define k7 0xFF42BD
#define k8 0xFF4AB5
#define k9 0xFF52AD
#define kSubmit 0xFFA25D


//rfid setup
#define SS_PIN 53
#define RST_PIN 5
MFRC522 rfid(SS_PIN, RST_PIN);  // Create MFRC522 instance.

//MFRC522::MIFARE_Key key;

int rfidCode[] = {163, 226, 242, 169}; //This is the stored UID
int codeRead = 0;
String uidString;

//everything else setup
int setDelay = 3000;

void setup() {
  // put your setup code here, to run once:
  lcd.init();  // initialize the lcd
  lcd.backlight();
  s.attach(serv);

  pinMode(button, INPUT_PULLUP);
  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);

  digitalWrite(ledR, LED_OFF);  // Make sure led is off
  digitalWrite(ledG, LED_OFF);  // Make sure led is off
  digitalWrite(ledB, LED_OFF); // Make sure led is off

  Serial.begin(19200);   // Initialize serial communications with PC
  SPI.begin();           // MFRC522 Hardware uses SPI protocol
  rfid.PCD_Init();    // Initialize MFRC522 Hardware

  irrecv.enableIRIn(); //enable ir receiver module

  /*  s.write(lockedPos);
    delay(1000);
    s.write(openPos);
    delay(3000);
    s.write(lockedPos);
    delay(1000);
    //s.detach();
  */

  lcd.print("Standby");      //What's written on the LCD you can change


  Serial.println(F("-------------------"));
  Serial.println(F("Everything Ready"));
  Serial.println(F("Waiting PICCs to be scanned"));
}

void loop() {
  // put your main code here, to run repeatedly:

  //rfid code
  if (  rfid.PICC_IsNewCardPresent())
  {
    readRFID();
  }
  delay(100);

  //ir test code
  if (irrecv.decode(&results)) //if the ir receiver module receiver data
  {
    Serial.print("irCode: "); //print"irCode: "
    Serial.print(results.value, HEX); //print the value in hexdecimal
    Serial.print(", bits: "); //print" , bits: "
    Serial.println(results.bits); //print the bits
    irrecv.resume(); // Receive the next value
  }

  //keypad code
  keypressed = customKeypad.getKey();               //Constantly waiting for a key to be pressed
  if (keypressed == '*') {                    // * to open the lock
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enter code");            //Message to show
    keypadInput();                          //Getting code function
    Serial.println(a);
    if (a == sizeof(code)) {     //The GetCode function assign a value to a (it's correct when it has the size of the code array)
      if (isLocked == HIGH) {
        granted(HIGH);                   //Open lock function if code is correct
      } else {
        granted(LOW);
      }
    } else {
      denied();
    }
    delay(2000);
    lcd.clear();
    lcd.print("Standby");
  }

  //ir lock
  if (results.value == kStart) {                    // * to open the lock
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enter code");            //Message to show
    irInput();                          //Getting code function
    Serial.println(a);
    if (a == sizeof(code)) {     //The GetCode function assign a value to a (it's correct when it has the size of the code array)
      if (isLocked == HIGH) {
        granted(HIGH);                   //Open lock function if code is correct
      } else {
        granted(LOW);
      }
    } else {
      denied();
    }
    /*if (a == sizeof(code)) {       //The GetCode function assign a value to a (it's correct when it has the size of the code array)
      if (isLocked == HIGH) {
        granted(HIGH);                   //Open lock function if code is correct
      } else {
        granted(LOW);
      }
      } else {
      lcd.clear();
      lcd.print("Wrong");          //Message to print when the code is wrong
      } */
    delay(2000);
    lcd.clear();
    lcd.print("Standby");             //Return to standby mode it's the message do display when waiting
  }

  //button code
  if (digitalRead(button) == LOW) {   //Opening by the push button
    if (isLocked == HIGH) {
      granted(HIGH);                   //Open lock function if code is correct
    } else {
      granted(LOW);
    }
  }
}
///////////////////////////////////////// Get PICC's UID ///////////////////////////////////
void readRFID()
{

  rfid.PICC_ReadCardSerial();
  Serial.print(F("\nPICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }


  Serial.println("Scanned PICC's UID:");
  printDec(rfid.uid.uidByte, rfid.uid.size);

  uidString = String(rfid.uid.uidByte[0]) + " " + String(rfid.uid.uidByte[1]) + " " + String(rfid.uid.uidByte[2]) + " " + String(rfid.uid.uidByte[3]);


  int i = 0;
  boolean match = true;
  while (i < rfid.uid.size)
  {
    if (!(rfid.uid.uidByte[i] == rfidCode[i]))
    {
      match = false;
    }
    i++;
  }

  if (match)
  {
    Serial.println("\nI know this card!");
    //printUnlockMessage();
    if (isLocked == HIGH) {
      granted(HIGH);                   //Open lock function if code is correct
    } else {
      granted(LOW);
    }
  } else
  {
    Serial.println("\nUnknown Card");
    denied();
  }


  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}

void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}

void granted (bool lockPos) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Welcome! :)");
  digitalWrite(ledB, LED_OFF);   // Turn off blue LED
  digitalWrite(ledR, LED_OFF);  // Turn off red LED
  digitalWrite(ledG, LED_ON);   // Turn on green LED
  if (lockPos) {
    s.write(openPos);     // Unlock door!
    isLocked = LOW;
    lockPos = LOW;
  } else {
    s.write(lockedPos);
    isLocked = HIGH;
    lockPos = HIGH;
  }
  delay(setDelay);          // Hold door lock open for given seconds
  digitalWrite(ledG, LED_OFF);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Standby");            //Message to show
}

void denied() {
  lcd.clear();
  lcd.print("Wrong >:(");          //Message to print when the code is wrong
  digitalWrite(ledB, LED_OFF);   // Turn off blue LED
  digitalWrite(ledR, LED_ON);  // Turn off red LED
  digitalWrite(ledG, LED_OFF);   // Turn on green LED
  delay(setDelay);
  digitalWrite(ledR, LED_OFF);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Standby");            //Message to show
}

void irInput() {
  i = 0;                    //All variables set to 0
  a = 0;
  j = 0;
  char k;
  //results.value = 0;

  //if (irrecv.decode(&results)) {
  while (results.value != kSubmit) {



    Serial.print("irCode: "); //print"irCode: "
    Serial.println(results.value, HEX); //print the value in hexdecimal
    //irrecv.resume(); // Receive the next value
    if (irrecv.decode(&results)) {
      Serial.println("test");
      if (results.value != kSubmit && results.value != kStart) {
        //        if (results.value == k0) {
        //          lcd.setCursor(j, 1);
        //          lcd.print("0");
        //          j++;
        //        }

        switch (results.value) {
          case k0:
            lcd.setCursor(j, 1);
            Serial.println("0");
            lcd.print("0");
            k = '0';
            j++;
            break;
          case k1:
            Serial.println("1");
            lcd.setCursor(j, 1);                                 //This to write "*" on the LCD whenever a key is pressed it's position is controlled by j
            lcd.print("1");
            k = '1';
            j++;
            break;
          case k2:
            Serial.println("2");
            lcd.setCursor(j, 1);                                 //This to write "*" on the LCD whenever a key is pressed it's position is controlled by j
            lcd.print("2");
            k = '2';
            j++;
            break;
          case k3:
            Serial.println("3");
            lcd.setCursor(j, 1);                                 //This to write "*" on the LCD whenever a key is pressed it's position is controlled by j
            lcd.print("3");
            k = '3';
            j++;
            break;
          case k4:
            Serial.println("4");
            lcd.setCursor(j, 1);                                 //This to write "*" on the LCD whenever a key is pressed it's position is controlled by j
            lcd.print("4");
            k = '4';
            j++;
            break;
          case k5:
            Serial.println("5");
            lcd.setCursor(j, 1);                                 //This to write "*" on the LCD whenever a key is pressed it's position is controlled by j
            lcd.print("5");
            k = '5';
            j++;
            break;
          case k6:
            Serial.println("6");
            lcd.setCursor(j, 1);                                 //This to write "*" on the LCD whenever a key is pressed it's position is controlled by j
            lcd.print("6");
            k = '6';
            j++;
            break;
          case k7:
            Serial.println("7");
            lcd.setCursor(j, 1);                                 //This to write "*" on the LCD whenever a key is pressed it's position is controlled by j
            lcd.print("7");
            k = '7';
            j++;
            break;
          case k8:
            Serial.println("8");
            lcd.setCursor(j, 1);                                 //This to write "*" on the LCD whenever a key is pressed it's position is controlled by j
            lcd.print("8");
            k = '8';
            j++;
            break;
          case k9:
            Serial.println("9");
            lcd.setCursor(j, 1);                                 //This to write "*" on the LCD whenever a key is pressed it's position is controlled by j
            lcd.print("9");
            k = '9';
            j++;
            break;
          case kSubmit:
            Serial.println("off");
            break;
        }
        if (results.value != 0xFFFFFFFF && results.value != kStart) {
          if (k == code[i] && i < sizeof(code)) {       //if the char typed is correct a and i increments to verify the next caracter
            a++;                                              //Now I think maybe I should have use only a or i ... too lazy to test it -_-'
            i++;
            Serial.println(a);
          } else {
            a--;                                               //if the character typed is wrong a decrements and cannot equal the size of code []
            Serial.println(a);
          }
          //irrecv.resume(); // Receive the next value
        }
      }
      irrecv.resume(); // Receive the next value
    }
  }
  irrecv.resume(); // Receive the next value
}
void keypadInput() {
  i = 0;                    //All variables set to 0
  a = 0;
  j = 0;

  while (keypressed != 'A') {                                   //The user press A to confirm the code otherwise he can keep typing
    keypressed = customKeypad.getKey();
    if (keypressed != NO_KEY && keypressed != 'A' ) {     //If the char typed isn't A and neither "nothing"
      lcd.setCursor(j, 1);                                 //This to write "*" on the LCD whenever a key is pressed it's position is controlled by j
      lcd.print(keypressed);
      Serial.println(keypressed);
      j++;
      if (keypressed == code[i] && i < sizeof(code)) {       //if the char typed is correct a and i increments to verify the next caracter
        a++;                                              //Now I think maybe I should have use only a or i ... too lazy to test it -_-'
        i++;
      }
      else
        a--;                                               //if the character typed is wrong a decrements and cannot equal the size of code []
    }
  }
  keypressed = NO_KEY;
}
