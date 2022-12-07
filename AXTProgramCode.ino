#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

LiquidCrystal_I2C lcd(0x27, 16, 2);
const char BUTTON_PIN = 8;
bool pressed = false;
//pins:
const int HX711_dout = 4; //mcu > HX711 dout pin
const int HX711_sck = 5; //mcu > HX711 sck pin

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAdress = 0;
unsigned long t = 0;
String myCmd;
int discrete;
long max_weight;
int remainder;
long weight_value;
long prev_weight = 0;

void setup() {
  Serial.begin(9600); delay(10);
  lcd.begin();
  lcd.backlight();
  LoadCell.begin();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  float calibrationValue; 
  
  EEPROM.get(calVal_eepromAdress, calibrationValue); // uncomment this if you want to fetch the calibration value from eeprom
  
  unsigned long stabilizingtime = 3000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Проверьте подключение");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
  }

 
  EEPROM.get(4, discrete);
  
  EEPROM.get(6, max_weight);
  
}

void loop() {
    
  if (Serial.available() > 0){
      myCmd = Serial.readStringUntil('\r');
      if (myCmd == "ghjjhg789hfk!hgfka7JHB45" ){
        Serial.println("ACCEPTED!");
        while (!LoadCell.update());
        calibrate();  
      } else if (myCmd == "t"){
          LoadCell.tareNoDelay();
          if (LoadCell.getTareStatus() == true) {}
        } 
  }

  if (digitalRead(BUTTON_PIN) == pressed){
     while(digitalRead(BUTTON_PIN) == pressed){
      //do nothing 
     }
     LoadCell.tareNoDelay();
     if (LoadCell.getTareStatus() == true) {}  
  }
  
  static boolean newDataReady = 0;
  const int serialPrintInterval = 800; //increase value to slow down serial print activity

  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float i = LoadCell.getData();
      long b = long(i);
      if (b <= max_weight){
        weight_value = long(b);

        if ( weight_value <= 20 && weight_value >= -20){
          weight_value = 0;  
        }

        remainder = weight_value % discrete;
        
        if (remainder != 0)
          weight_value = weight_value + discrete - remainder;

       /* if (weight_value < 81 && weight_value > -120){
            Serial.println(0);
          }else{
              Serial.println(weight_value);
            }*/
        /*
        if ( weight_value <= 20 && weight_value >= -20){
          weight_value = 0;  
        }*/

        if ( prev_weight == 0 ){
          Serial.println(weight_value);
          lcd.clear();
          lcd.print(weight_value);      
        } 
        else {
          if (prev_weight == weight_value && weight_value >= -100 && weight_value <= 50){
              LoadCell.tare();
          } else {
            Serial.println(weight_value);
            lcd.clear();
            lcd.print(weight_value);
          }
        }

       prev_weight = weight_value;
        
        
      } else{
          Serial.println("ERROR!");
          lcd.clear();
          lcd.print("ERROR!");
        }
      


      
      newDataReady = 0;
      t = millis();
    }
  }



  // check if last tare operation is complete:
  

}

void calibrate() {
  boolean _resume = false;
  String user_action;
  
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      if (Serial.available() > 0) {
        user_action = Serial.readStringUntil('\r');
        if (user_action == "1") LoadCell.tareNoDelay();
      }
    }
    if (LoadCell.getTareStatus() == true) {
      Serial.println("STEP 1 COMPLETED");
      _resume = true;
    }
  }

  float known_mass = 0;
  _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      known_mass = Serial.parseFloat();
      if (known_mass != 0) {
        _resume = true;
      }

    }
  }

  LoadCell.refreshDataSet(); //refresh the dataset to be sure that the known mass is measured correct
  float newCalibrationValue = LoadCell.getNewCalibration(known_mass); //get the new calibration value

  _resume = false;
/*
#if defined(ESP8266)|| defined(ESP32)
  EEPROM.begin(512);
#endif
  EEPROM.put(calVal_eepromAdress, newCalibrationValue);
  
#if defined(ESP8266)|| defined(ESP32)
  EEPROM.commit();
#endif
  EEPROM.get(calVal_eepromAdress, newCalibrationValue); */

  
  Serial.println("STEP 2 COMPLETED");
 
  int discrete_c = 0;
  _resume = false;
  while (_resume == false) {
    
    if (Serial.available() > 0) {
      discrete_c = Serial.parseInt();
      if (discrete_c != 0)
        _resume = true;
    }
  }
  _resume = false;
/*
  EEPROM.put(4, discrete_c);

  discrete = discrete_c; */

  Serial.println("STEP 3 COMPLETED");

  long max_weight_c = 0;
  _resume = false;
  while (_resume == false) {
    
    if (Serial.available() > 0) {
      max_weight_c = Serial.parseInt();
      if (max_weight_c != 0)
        _resume = true;
    }
  }
  _resume = false;
  /*
  EEPROM.put(6, max_weight_c);

  max_weight = max_weight_c; */


#if defined(ESP8266)|| defined(ESP32)
  EEPROM.begin(512);
#endif
  EEPROM.put(calVal_eepromAdress, newCalibrationValue);
  
#if defined(ESP8266)|| defined(ESP32)
  EEPROM.commit();
#endif
  EEPROM.get(calVal_eepromAdress, newCalibrationValue);

  EEPROM.put(4, discrete_c);
  discrete = discrete_c;

  EEPROM.put(6, max_weight_c);
  max_weight = max_weight_c;
  
  Serial.println("FINISH");
  

}
