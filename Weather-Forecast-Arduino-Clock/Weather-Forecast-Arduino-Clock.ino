#include <Wire.h>            // I2C
#include <LCD5110_Graph.h>   // LCD5110 Module
#include <DS1307.h>       // DS1307 RTC
#include <Adafruit_BME280.h> // BME
// ========================================

// PINs Define
// ========================================
// [ LCD5110 Module ]
#define LCD_LIGHT 7   // default: 7
#define LCD_SCK   8   // default: 8
#define LCD_MOSI  9   // default: 9
#define LCD_DC    10  // default: 10
#define LCD_CS    11  // default: 11
#define LCD_RST   12  // default: 12
// -----------------
// [ LDR ]
#define LDR_PIN   1   // default: 1
#define LDR_VCC   A3  // default: A3
// -----------------
// [ Motor ]
#define MOTOR_PIN 5   // default: 5
// -----------------
// [ Buzzer ]
#define BUZZ_PIN  6   // must be PWM, default: 6
// -----------------
// [ Voltmeter ]
#define VOLT_PIN  0   // default: 0
// -----------------
// [ Buttons ]
#define BUT_OK    2   // default: 2
#define BUT_UP    3   // default: 3
#define BUT_DOWN  4   // default: 4
// -----------------
// [ BME ]
#define BME_I2C   0x76  //default: 0x76
// -----------------
// ========================================

// System Memory Storage
// ========================================
// Main array
boolean backlight = false; // indicates backlight
byte voltagePercent = 100;
byte old_min = 0;
const byte motorSpeed = 80;
byte lightIntensity[2] = {0, 0};

// Init Link Data with Menu Item Value as Below
// CurHour [11], CurMin [12], AlarmHour [18], AlarmMin [19], AlarmActive [5], 
// MelodyAlarm [8], BellAlarm [9], WeatherMeasure [14], WeatherWarning[15], ShowDate [13]
const byte LinkData[] = {11, 12, 18, 19, 5, 8, 9, 15, 16, 13};

// Button Array
// { AnyKey, Up, Down, OK }
bool ButtonStatus[4] = {false, false, false, false};
// -----------------

// -----------------
// Forecast array
int my_forecast[] = {0,0,0,0,0};
/*
 * 0 - Current pressure
 * 1 - Difference Max - Min
 * 2 - Max. Pressure
 * 3 - Min. Pressure
 * 4 - Pressure Dynamic (1 - Up, 2 - Down, 0 - Equal)
*/
byte forecast_count = 0;
int temperature = 0;
byte humidity;
float pressure = 0;
byte old_sel = 1;
const int altitude = 155; // Default: 155
// -----------------
// ========================================

// Init Modules & Load Additional Functions
// ========================================
// LCD
LCD5110 lcd(LCD_SCK, LCD_MOSI, LCD_DC, LCD_RST, LCD_CS);
extern unsigned char SmallFont[];
extern unsigned char BigNumbers[];
// -----------------

// ICONs Attach
extern uint8_t IconTemperature[];
extern uint8_t IconHumidity[];
extern uint8_t IconBattery[];
extern uint8_t IconClear[];
extern uint8_t IconCloudy[];
extern uint8_t IconRainy[];
extern uint8_t IconExit[];
extern uint8_t IconOn[];
extern uint8_t IconOff[];
extern uint8_t IconClock[];
extern uint8_t IconAlarm[];
extern uint8_t IconAlarmMini[];
extern uint8_t IconWeather[];
extern uint8_t IconSound[];
// -----------------

// I2C: A4 (SDA) & A5 (SCL)
// DS1307 RTC Time Module
   DS1307 tm(SDA, SCL);
   Time rtc_time;
// BME280 Sensors
   Adafruit_BME280 bme;
// -----------------

// Music
#include "sounds.h"  // Load Music Notes & Functions
// -----------------

// Load Main Functions
#include "functions.h"  // Load Functions
// -----------------

// ========================================
// -=[ SETUP Section ]=-
// ========================================
void setup() {
  // Start RTC DS1307  
  tm.begin();
  rtc_time = tm.getTime();

  // Init Menu Items
  MMenus[0] = (Menu_Struct) {"Exit",      0, 0, 0, IconExit};
  MMenus[1] = (Menu_Struct) {"Alarm",     3, 0, 0, IconAlarm};
  MMenus[2] = (Menu_Struct) {"Set Clock", 2, 0, 0, IconClock};
  MMenus[3] = (Menu_Struct) {"Weather",   3, 0, 0, IconWeather};
  // Alarm Menu [1]
  MMenus[4] = (Menu_Struct) {"Set Alarm",    2, 0, 1};
  MMenus[5] = (Menu_Struct) {"Active",       1, 1, 1};
  MMenus[6] = (Menu_Struct) {"Melody/Bells", 3, 0, 1, IconSound};
  MMenus[7] = (Menu_Struct) {"Main Menu",    0, 0, 1};
  // Melody Menu [6]
  MMenus[8]  = (Menu_Struct) {"Melody",    1, 0, 6};
  MMenus[9]  = (Menu_Struct) {"Bells",     1, 0, 6};
  MMenus[10] = (Menu_Struct) {"Main Menu", 0, 0, 6};
  // Clock Menu [2]
  MMenus[11] = (Menu_Struct) {"Hours",     4, 0, 2};
  MMenus[12] = (Menu_Struct) {"Minutes",   4, 0, 2};
  MMenus[13] = (Menu_Struct) {"Show Date",   1, 1, 2};
  MMenus[14] = (Menu_Struct) {"Save&Exit", 0, 0, 2};
  // Weather Menu [3]
  MMenus[15] = (Menu_Struct) {"Metric",     1, 0, 3};
  MMenus[16] = (Menu_Struct) {"Warnings",   1, 0, 3};
  MMenus[17] = (Menu_Struct) {"Main Menu",  0, 0, 3};
  // Set Alarm Time [4]
  MMenus[18] = (Menu_Struct) {"Hours",     4, 6, 4};
  MMenus[19] = (Menu_Struct) {"Minutes",   4, 0, 4}; 
  MMenus[20] = (Menu_Struct) {"Save&Exit", 0, 0, 4}; 

  // Load Default value from RTC Module Memory
  for (byte i = 0; i < (sizeof(LinkData)/sizeof(byte)); i++) { MMenus[LinkData[i]].value = tm.peek(i);}
  // ---------------------------------

  // Setup LCD
  lcd.InitLCD();
  lcd.clrScr();
  lcd.setFont(SmallFont);
  pinMode(LCD_LIGHT, OUTPUT); // define backlight pin as output
  //LCDLight(false); //Turn Backlight OFF

  

  // Setup interrupt button
  pinMode(BUT_OK, INPUT);
  // RISING to trigger when the pin goes from low to high
  attachInterrupt(digitalPinToInterrupt(BUT_OK), MenuButtonPressed, RISING);
  
  // Setup UP and Down buttons which use internal PULLUP
  pinMode(BUT_UP, INPUT_PULLUP);
  pinMode(BUT_DOWN, INPUT_PULLUP);


  // Setup buzzer
  pinMode(BUZZ_PIN, OUTPUT); // Set buzzer - pin 6 as an output
  //playMusic();

  //Setup Light Sensor
  pinMode(LDR_VCC, OUTPUT); // define this pin as output


  // Setup motor
  pinMode(MOTOR_PIN, OUTPUT); // motor

  //lcd.print("LOADING...", CENTER, 20);
  //lcd.update();

  // Setup BME weather sensor
  if (!bme.begin(BME_I2C)) {
      lcd.clrScr();
      lcd.print("!!!ERROR!!!", CENTER, 10);
      lcd.print("Check BME...", CENTER, 20);
      lcd.update();
      while (bme.begin(BME_I2C));
  }
  MenuButton = false;  
  DebounceTime = millis();
 
  BMEGetData();
  old_min = rtc_time.min;
}
// ========================================
// -=[ LOOP ]=-
// ========================================
void loop() {
  CheckLCDLigh();
  ShowClock();
  KeepCalm(10);
  if ((rtc_time.min % 12 == 0)&&(old_min != rtc_time.min) )
    {
      BMEGetData();
      LCDLight(false);
      old_min = rtc_time.min;
    }
  drawTemperature();
  KeepCalm(2000);
  drawHumidity();
  KeepCalm(2000);
  if (rtc_time.min % 3 == 0) 
    {
      drawVoltage();  
      KeepCalm(3000);  
    }
  drawWeather();
  KeepCalm(3000);
}
// ========================================
