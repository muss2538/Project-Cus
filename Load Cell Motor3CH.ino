/*
  Mr.Marut Buddee Electronics RMUTI KKC
  Created : 26 มิ.ย. 2562
*/


#include <avr/wdt.h>
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);
//LiquidCrystal_I2C lcd(0x3F, 20, 4);
#include <Adafruit_ADS1015.h>
Adafruit_ADS1115 ads;
#include <Keypad.h>

#define RelayA 10
#define RelayB 11
#define RelayC 12
#define RelayD 13
#define SwSelector 9
#define SwMotorA A0
#define SwMotorB A1
#define SwMotorC A2

#define ROWS 4
#define COLS 3

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[ROWS] = {5, 6, 7, 8}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {2, 3, 4}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);


bool STa = LOW , STb = LOW , STc = LOW;
unsigned long previousMillis = 0;
const int interval = 8;
unsigned long adc0, adc1;
int16_t Weight0, Weight1;
//unsigned long ZeroWeight0 = 7615 , ZeroWeight1 = 7503;  //7911=296  ,7771=268 =564
unsigned long ZeroWeight0 = 99999 , ZeroWeight1 = 99999;
byte CountAver = 0;

String Key = "";
byte Mode = 0;
byte Stepp = 0;
byte saveStep = 0;
int Weight;
int setWeight[4];
int SaveWeight[4];
int Total = 0;
bool SW;
void setup() {
  //Serial.begin(9600);
  lcd.begin();
  ads.begin();
  lcd.clear();
  pinMode(RelayA, OUTPUT); digitalWrite(RelayA, STa);
  pinMode(RelayB, OUTPUT); digitalWrite(RelayB, STb);
  pinMode(RelayC, OUTPUT); digitalWrite(RelayC, STc);
  pinMode(RelayD, OUTPUT); digitalWrite(RelayC, LOW);
  pinMode(SwSelector, INPUT);
  pinMode(SwMotorA, INPUT);
  pinMode(SwMotorB, INPUT);
  pinMode(SwMotorC, INPUT);
  SaveWeight[0] = EEPROM.read(0);
  SaveWeight[1] = EEPROM.read(1);
  SaveWeight[2] = EEPROM.read(2);
  SaveWeight[3] = EEPROM.read(3);
  lcd.setCursor(0, 0);  lcd.print("  Project RMUTI KKC ");
  lcd.setCursor(0, 1);  lcd.print("Address[0] : " + String(SaveWeight[0]));
  lcd.setCursor(0, 2);  lcd.print("Address[1] : " + String(SaveWeight[1]));
  lcd.setCursor(0, 3);  lcd.print("Address[2] : " + String(SaveWeight[2]));
  delay(3000);
  lcd.clear();
  wdt_enable(WDTO_8S);
}
void loop() {
  SW = digitalRead(SwSelector);
  if (SW == HIGH) { //Mode Man
    STa = LOW; STb = LOW; STc = LOW;
    lcd.setCursor(0, 0);  lcd.print("  MODE:MANUAL  --kg ");
    lcd.setCursor(0, 1);  lcd.print("    MotorA = OFF    ");
    lcd.setCursor(0, 2);  lcd.print("    MotorB = OFF    ");
    lcd.setCursor(0, 3);  lcd.print("    MotorC = OFF    ");

    while (SW == HIGH) {
      ReadWeight();
      lcd.setCursor(0, 0);  lcd.print("  Mode:Manual " + String(Total) + " kg  ");
    
        switch (SwMotorARead(SwMotorA)) {
        case 1 :
          STa = HIGH;
          delay(500);
          lcd.setCursor(0, 1);  lcd.print("    MotorA = ON     ");
          break;
        case 2 :
          STa = LOW;
          delay(500);
          lcd.setCursor(0, 1);  lcd.print("    MotorA = OFF    ");
          break;
        default:
          break;
        }
        switch (SwMotorBRead(SwMotorB)) {
        case 1 :
          STb = HIGH;
          delay(500);
          lcd.setCursor(0, 2);  lcd.print("    MotorB = ON     ");
          break;
        case 2 :
          STb = LOW;
          delay(500);
          lcd.setCursor(0, 2);  lcd.print("    MotorB = OFF    ");
          break;
        default:
          break;
        }
        switch (SwMotorCRead(SwMotorC)) {
        case 1 :
          STc = HIGH;
          delay(500);
          lcd.setCursor(0, 3);  lcd.print("    MotorC = ON     ");
          break;
        case 2 :
          STc = LOW;
          delay(500);
          lcd.setCursor(0, 3);  lcd.print("    MotorC = OFF    ");
          break;
        default:
          break;
        }
      digitalWrite(RelayA, STa);
      digitalWrite(RelayB, STb);
      digitalWrite(RelayC, STc);
      delayMicroseconds(10);
      SW = digitalRead(SwSelector);
      wdt_reset();
    }
  }
  if (SW == LOW) { //Mode Auto
    STa = LOW; STb = LOW; STc = LOW;
    digitalWrite(RelayA, STa);
    digitalWrite(RelayB, STb);
    digitalWrite(RelayC, STc);
    while (SW == LOW) {
      if ((SaveWeight[0] + SaveWeight[1] + SaveWeight[2]) != 0) {//ถ้ามีการตั้งค่าแล้ว
        Mode = 1;
        Loop_Auto();
        Loop_Set();
      }
      if ((SaveWeight[0] + SaveWeight[1] + SaveWeight[2]) == 0) {//ถ้ายังไม่มีการตั้งค่า
        Mode = 0;
        LCD_Dis(Mode);
        Loop_Set();
      }
      SW = digitalRead(SwSelector);
      wdt_reset();
    }
  }
  //End
  wdt_reset();
}
void Loop_Set() {
  char customKey = customKeypad.getKey();
  if (customKey == '*') {
    Mode = 2;
    STa = LOW; STb = LOW; STc = LOW;
    digitalWrite(RelayA, STa);
    digitalWrite(RelayB, STb);
    digitalWrite(RelayC, STc);
    saveStep = 0;
  }
  while ((Mode == 2) && (SW == LOW)) {
    LCD_Dis(Mode);
    char customKey = customKeypad.getKey();
    if (customKey == '#') {
      SaveWeight[saveStep] = Key.toInt();
      Key = "";
      saveStep++;
      LCD_Dis(Mode);
      delay(500);
    }
    if ((customKey >= '0') && (customKey <= '9')) {
      Key += (char)customKey;
      LCD_Dis(Mode);
      delay(500);
    }
    if (Key.toInt() >= 50) {
      lcd.clear();
      lcd.setCursor(0, 1);  lcd.print(" Weights Over Limit ");
      lcd.setCursor(0, 2);  lcd.print("  Resetting Again.  ");
      delay(1000);
      Key = "";
      SaveWeight[0] = 0; SaveWeight[1] = 0; SaveWeight[2] = 0;
      saveStep = 0;
    }
    if (saveStep == 4) {
      EEPROM.write(0, SaveWeight[0]);
      EEPROM.write(1, SaveWeight[1]);
      EEPROM.write(2, SaveWeight[2]);
      EEPROM.write(3, SaveWeight[3]);
      lcd.clear();
      Key = "";
      saveStep = 0;
      lcd.setCursor(0, 2);  lcd.print("   Save Setting..   ");
      delay(1000);
      Mode = 1;

    }
    SW = digitalRead(SwSelector);
    wdt_reset();
  }

}
void Loop_Auto() {
  Stepp = 1;
  setWeight[0] = SaveWeight[0];
  setWeight[1] = SaveWeight[0] + SaveWeight[1];
  setWeight[2] = SaveWeight[0] + SaveWeight[1] + SaveWeight[2];
  setWeight[3] = SaveWeight[3];
  while ((Mode == 1) && (SW == LOW)) {
    ReadWeight();
    Weight = Total;
    if (Stepp == 1) {
      if (Weight < setWeight[Stepp - 1]) {
        STa = HIGH;
        LCD_Dis(Mode);
        delayMicroseconds(10);
      }
      if (Weight >= setWeight[Stepp - 1]) {
        STa = LOW;
        Stepp++;
      }
      digitalWrite(RelayA, STa);
    }
    if (Stepp == 2) {
      if (Weight < setWeight[Stepp - 1]) {
        STb = HIGH;
        LCD_Dis(Mode);
        delayMicroseconds(10);
      }
      if (Weight >= setWeight[Stepp - 1]) {
        STb = LOW;
        Stepp++;
      }
      digitalWrite(RelayB, STb);
    }
    if (Stepp == 3) {
      if (Weight < setWeight[Stepp - 1]) {
        STc = HIGH;
        LCD_Dis(Mode);
        delayMicroseconds(10);
      }
      if (Weight >= setWeight[Stepp - 1]) {
        STc = LOW;
        Stepp++;
      }
      digitalWrite(RelayC, STc);
    }
    if (Stepp == 4) {
      if (Weight > setWeight[Stepp - 1]) {
        STa = LOW;
        STb = LOW;
        STc = LOW;
        LCD_Dis(Mode);
        delayMicroseconds(10);
      }
      if (Weight <= setWeight[Stepp - 1]) {
        Stepp = 1;
        delay(1000);
      }
      digitalWrite(RelayA, STa);
      digitalWrite(RelayB, STb);
      digitalWrite(RelayC, STc);
    }
    char customKey = customKeypad.getKey();
    if (customKey == '*') {
      Mode = 2;
      STa = LOW; STb = LOW; STc = LOW;
      digitalWrite(RelayA, STa);
      digitalWrite(RelayB, STb);
      digitalWrite(RelayC, STc);
      saveStep = 0; Stepp = 0;
      lcd.clear();
      delay(1000);
    }
    SW = digitalRead(SwSelector);
    wdt_reset();
  }
}
void LCD_Dis(byte var) {
  switch (var) {
    case 0 :
      lcd.setCursor(0, 1);  lcd.print("    MODE : AUTO     ");
      lcd.setCursor(0, 2);  lcd.print("   Please Setting   ");
      lcd.setCursor(0, 3);  lcd.print(" Press the button * ");
      lcd.setCursor(0, 0);  lcd.print("                    ");
      break;
    case 1  :
      lcd.setCursor(0, 0);  lcd.print("    MODE : AUTO     ");
      lcd.setCursor(0, 1);  lcd.print("     Step : " + String(Stepp) + "       ");
      lcd.setCursor(0, 2);  lcd.print("   " + String(setWeight[Stepp - 1]) + " KG : " + String(Weight) + " KG    ");
      lcd.setCursor(0, 3);  lcd.print("    Please wait     ");
      break;
    case 2  :
      lcd.setCursor(0, 0);  lcd.print("   MODE : Setting   ");
      lcd.setCursor(0, 1);  lcd.print("     Step : " + String(saveStep + 1) + "       ");
      lcd.setCursor(0, 2);  lcd.print(" Enter Weight : " + Key + "kg ");
      lcd.setCursor(0, 3);  lcd.print("  Push # For Save   ");
      break;
    default:
      break;
  }
}
void ReadWeight() {

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    adc0 = adc0 + ads.readADC_SingleEnded(0);
    adc1 = adc1 + ads.readADC_SingleEnded(1);
    CountAver++;
    if (CountAver == 10) {
      Weight0 = adc0 / 10;
      Weight1 = adc1 / 10;
      if ((ZeroWeight0 >= Weight0) || (ZeroWeight1 >= Weight1)) {
        ZeroWeight0 = Weight0;
        ZeroWeight1 = Weight1;
      }
      adc0 = 0;                                                                          
      adc1 = 0;
      CountAver = 0;
      double WSum = (Weight0 + Weight1) - (ZeroWeight0 + ZeroWeight1);
      WSum = (0.1129*WSum) + 0.0708 -(0.00008*pow(WSum,2));
      Total = round(WSum);

    }
  }
}
byte SwMotorARead(byte pin) {
  int a = analogRead(pin);
  if ((a >= 310) && (a <= 460)) {
    //Stop
    return 1;
  }
  else if ((a > 490) && (a < 600)) {
    //Start
    return 2;
  }
  else {
    return 0;
  }
};
byte SwMotorBRead(byte pin) {
  int a = analogRead(pin);
  if ((a >= 380) && (a <= 460)) {
    //Stop
    return 1;
  }
  else if ((a > 490) && (a < 600)) {
    //Start
    return 2;
  }
  else {
    return 0;
  }
};
byte SwMotorCRead(byte pin) {
  int a = analogRead(pin);
  if ((a >= 380) && (a <= 460)) {
    //Stop
    return 1;
  }
  else if ((a > 490) && (a < 600)) {
    //Start
    return 2;
  }
  else {
    return 0;
  }
};
