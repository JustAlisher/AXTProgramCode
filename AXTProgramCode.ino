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

const int calVal_eepromAddress = 0;
const int discrete_eepromAddress = 4;
const int max_weight_eepromAddress = 6;
const int number_of_calibrations_eepromAddress = 10;
const int activation_key_eepromAddress = 12;

unsigned long t = 0;
String myCmd;
int discrete;
long max_weight;
int remainder;
long weight_value;
long prev_weight = 0;
int number_of_calibrations;
char activation_key;

void setup() {
  //Check the key
  EEPROM.get(activation_key_eepromAddress, activation_key);

  if(activation_key!='d'){
    EEPROM.put(calVal_eepromAddress, 1.05);
    EEPROM.put(discrete_eepromAddress, 1);
    EEPROM.put(max_weight_eepromAddress, 1000000);
    EEPROM.put(number_of_calibrations_eepromAddress, 0);
    EEPROM.put(activation_key_eepromAddress, 'd');
  }

  
  Serial.begin(9600); delay(10);
  lcd.begin();
  lcd.backlight();
  LoadCell.begin();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  float calibrationValue; 
  
  EEPROM.get(calVal_eepromAddress, calibrationValue); // uncomment this if you want to fetch the calibration value from eeprom
  
  unsigned long stabilizingtime = 3000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("2222222");
    lcd.clear();
    lcd.print("Проверьте подключение проводов");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
  }

 
  EEPROM.get(discrete_eepromAddress, discrete);
  
  EEPROM.get(max_weight_eepromAddress, max_weight);

  EEPROM.get(number_of_calibrations_eepromAddress, number_of_calibrations);
  
}

void loop() {
    
  if (Serial.available() > 0){
      myCmd = Serial.readStringUntil('\r');
      if (myCmd == "ghjjhg789hfk!hgfka7JHB45" ){
        Serial.println("ACCEPTED!");
        Serial.println(number_of_calibrations);
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
          Serial.println("11111111");
          lcd.clear();
          lcd.print("ERROR!");
        }
      
      newDataReady = 0;
      t = millis();
    }
  }

  

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


#if defined(ESP8266)|| defined(ESP32)
  EEPROM.begin(512);
#endif
  EEPROM.put(calVal_eepromAddress, newCalibrationValue);
  
#if defined(ESP8266)|| defined(ESP32)
  EEPROM.commit();
#endif
  EEPROM.get(calVal_eepromAddress, newCalibrationValue);

  EEPROM.put(discrete_eepromAddress, discrete_c);
  discrete = discrete_c;

  EEPROM.put(max_weight_eepromAddress, max_weight_c);
  max_weight = max_weight_c;

  number_of_calibrations = number_of_calibrations + 1;
  EEPROM.put(number_of_calibrations_eepromAddress, number_of_calibrations);
  
  Serial.println("FINISH");
  

}
