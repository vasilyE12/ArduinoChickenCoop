// chicken coop controller with menue, encoder, timer modual and motor driver
// endcoder pins
#define CLK 3
#define DT 2
#define SW 0

#include "GyverEncoder.h"
Encoder enc1(CLK, DT, SW);  // for working with the button

#include <microWire.h>
#include <microLiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);
#include <microDS3231.h>
MicroDS3231 rtc; //for the screen and the timer modual

//stuff for the button and the motor

#define PIN 7        // the button is connected (PIN --- button --- GND)
#include "GyverButton.h"
GButton butt1(PIN);



//motor stuff is here
#include <SparkFun_TB6612.h>

#define BIN1 10
#define BIN2 12
#define PWMB 11
#define STBY 99

const int offsetA = 1;
const int offsetB = 1;
Motor motor2 = Motor(BIN1, BIN2, PWMB, offsetB, STBY);

// veriables to display on screen
boolean val1 = true;
int Time1 = 8; //open time
boolean val3 = true;
int Time2 = 21; //close time
int DriverTimeOut = 30;
boolean doorStat = false;
int refresh =0;

int theDayOpen = 0; //veriable used to check if the open/close task has been complited today 
int theDayClose = 0;


int8_t arrowPos = 0;  // display errow position


//to turn off the screen back light to save power
boolean backlState = true; //if screen light is turn on or of
int backlTimer = 0;
int backlMin = 0;

void setup() {
  Serial.begin(9600);

  if (rtc.lostPower()) {  //  if power is lost
    rtc.setTime(COMPILE_TIME);  // set time
  }
  // sec, min, hour, day, month , year
  //rtc.setTime(0, 20, 14, 26, 6, 2021); // setting the time by hand if needed to adjust
  
  enc1.setType(TYPE2);

  
  lcd.init();
  lcd.backlight();
  
  printGUI();   // print the interface

  //more button set up stuff
  butt1.setDebounce(50);        // настройка антидребезга (по умолчанию 80 мс)
  butt1.setTimeout(300);        // настройка таймаута на удержание (по умолчанию 500 мс)
  butt1.setClickTimeout(600);   // настройка таймаута между кликами (по умолчанию 300 мс)

  // HIGH_PULL - кнопка подключена к GND, пин подтянут к VCC (PIN --- КНОПКА --- GND)
  // LOW_PULL  - кнопка подключена к VCC, пин подтянут к GND
  butt1.setType(HIGH_PULL);

  // NORM_OPEN - нормально-разомкнутая кнопка
  // NORM_CLOSE - нормально-замкнутая кнопка
  butt1.setDirection(NORM_OPEN); //type of the button norm open is when the button is open when not pressed
  backlOn();
}

void loop() {
  
  enc1.tick();
  butt1.tick();  // refresh
  if (enc1.isTurn()) {  //for every enoder turn
    backlOn();
    lcd.clear();        // clear the display
    
    if (enc1.isLeft()) {
      arrowPos++;
      if (arrowPos >= 5) arrowPos = 4; //cursure position is limited
    }
    if (enc1.isRight()) {
      arrowPos--;
      if (arrowPos < 0) arrowPos = 0;  //cursure position is limited
    }

    // when encoder is rotated left
    if (enc1.isLeftH()) {
      switch (arrowPos) {
        case 0: val1 = true;
          break;
        case 1: if(Time1<23){
          Time1++;
        }
          break;
        case 2: val3 = true;
          break;
        case 3: if(Time2<23){
          Time2++;
        }
          break;
        case 4: if(DriverTimeOut <60){
          DriverTimeOut++;
        }
          break;
      }
    }

    // when encoder is rotated right
    if (enc1.isRightH()) {
      switch (arrowPos) {
        case 0: val1 = false;
          break;
        case 1: if(Time1 != 0){
          Time1--;
        }
          break;
        case 2: val3 = false;
          break;
        case 3: if(Time2 != 0){
          Time2--;
        }
          break;
        case 4: if(DriverTimeOut !=0){
          DriverTimeOut--;
        }
          break;
      }
    }
    
    printGUI();  
    printTime();
  }
  
  if(refresh !=rtc.getSeconds()){
    refresh = rtc.getSeconds();
    printTime();
  }
  
 if (butt1.isPress()){ Serial.println("Press"); 
    backlOn();
    Serial.println(DriverTimeOut); 
    if(doorStat == false){
      if(DriverTimeOut<=30){
        motor2.drive(255,DriverTimeOut*1000);//opening the door
        motor2.brake();
        doorStat = true;
      }else{
        motor2.drive(255,30*1000);//opening the door
        motor2.brake();
        motor2.drive(255,(DriverTimeOut-30)*1000);//opening the door
        motor2.brake();
        doorStat = true;
      }
    }else{
      if(DriverTimeOut<=30){
        motor2.drive(-255,DriverTimeOut*1000);//closing the door
        motor2.brake();
        doorStat = false;
      }else{
        motor2.drive(-255,30*1000);//closing the door
        motor2.brake();
        motor2.drive(-255,(DriverTimeOut-30)*1000);//opening the door
        motor2.brake();
        doorStat = false;
      }
    }
    lcd.clear();  
    printGUI();   // выводим интерфейс
    printTime();
  }
 if(rtc.getHours() == Time1 && doorStat == false && theDayOpen != rtc.getDay() && val1 == true){//if time to open door and door is closed
    if(DriverTimeOut<=30){
      motor2.drive(255,DriverTimeOut*1000);//opening the door
      motor2.brake();
    }else{
      motor2.drive(255,30*1000);//opening the door
      motor2.brake();
      motor2.drive(255,(DriverTimeOut-30)*1000);//opening the door
      motor2.brake();
    }
    theDayOpen = rtc.getDay();
    doorStat = true;//open the door
    lcd.clear();  
    printGUI();   // update the screen
    printTime();
 }
 if(rtc.getHours() == Time2 && doorStat == true && theDayClose != rtc.getDay() && val3 == true){//if time to close the door and door is open
    if(DriverTimeOut<=30){
      motor2.drive(-255,DriverTimeOut*1000);//closing the door
      motor2.brake();
    }else{
      motor2.drive(-255,30*1000);//closing the door
      motor2.brake();
      motor2.drive(-255,(DriverTimeOut-30)*1000);//closing the door
      motor2.brake();
    }
    theDayClose = rtc.getDay();
    doorStat = false;//close the door
    lcd.clear();  
    printGUI();   // update the screen
    printTime();
 }
 backlTick();
 Serial.println(backlTimer);
}

void backlOn() {//when you call on this method it will turn on the back light
  backlState = true;
  backlMin = rtc.getMinutes();
  lcd.backlight();//call to turn on the back light
}

void backlTick() {//if timer runs out the backlight will turn off
  if (backlState && backlMin != rtc.getMinutes()) {
    backlMin = rtc.getMinutes();
    backlState = false;
    lcd.noBacklight();//call to turn off the back light
  }
}
//prints exact time
void printTime(){
  lcd.setCursor(6, 3);
  lcd.print("Time ");
  lcd.print(rtc.getHours());
  lcd.print(":");
  lcd.print(rtc.getMinutes());
  lcd.print(":");
  lcd.print(rtc.getSeconds());
  lcd.print(" ");
}

void printGUI() { //prints all of the veriables on the screen
  lcd.setCursor(0, 0); lcd.print("Use:"); 
  if(val1 == true){
    lcd.print("Yes");
  }
  if(val1 == false){
    lcd.print("No");
  }
  lcd.setCursor(8, 0); lcd.print("openT:"); lcd.print(Time1);
  if(Time1 <10){
    lcd.setCursor(15, 0);
    lcd.print("h");
  }else{
    lcd.setCursor(16, 0);
    lcd.print("h");
  }
  lcd.setCursor(0, 1);  lcd.print("Use:"); 
  if(val3 == true){
    lcd.print("Yes");
  }
  if(val3 == false){
    lcd.print("No");
  }
  lcd.setCursor(8, 1); lcd.print("closeT:"); lcd.print(Time2);
  if(Time2 <10){
    lcd.setCursor(16, 1);
    lcd.print("h");
  }else{
    lcd.setCursor(17, 1);
    lcd.print("h");
  }
  lcd.setCursor(0, 2); lcd.print("DrivOut:");lcd.print(DriverTimeOut);
  if(DriverTimeOut <10){
    lcd.setCursor(9, 2);
    lcd.print("s");
  }else{
    lcd.setCursor(10, 2);
    lcd.print("s");
  }
  lcd.setCursor(11, 2);
  if(doorStat == true){
    lcd.print("DoorOpen");
  }else{
    lcd.print("DoorClos");
  }
  
  // errow display
  switch (arrowPos) {
    case 0: lcd.setCursor(3, 0);
      break;
    case 1: lcd.setCursor(13, 0);
      break;
    case 2: lcd.setCursor(3, 1);
      break;
    case 3: lcd.setCursor(14, 1);
      break;
    case 4: lcd.setCursor(7, 2);
  }
  lcd.write(126);   // display the errow
}
