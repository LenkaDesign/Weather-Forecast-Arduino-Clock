// LCD Backlight Turn On/Off
// -----------------
void LCDLight(bool on)
{
  if (on == false) {
  digitalWrite(LCD_LIGHT, HIGH);
  backlight = false;
  } else {
    digitalWrite(LCD_LIGHT, LOW);
    backlight = true; 
  }
}
// ========================================

// Light Control
// -----------------
void CheckLCDLigh() {
  digitalWrite(LDR_VCC, HIGH); //Turn Light Sensor ON
  delay(200);
  lightIntensity[1] = map(analogRead(LDR_PIN), 0, 1023, 0, 250);
  digitalWrite(LDR_VCC, LOW); //Turn Light Sensor OFF to save power
  if ((abs(lightIntensity[1]-lightIntensity[0]) > 12) && (backlight == false)) // Check Intensity
    {
      LCDLight(true); // Off
      lightIntensity[0] = lightIntensity[1];
    } //else if (forecast_count % 2 == 0){ LCDLight(false); }
}
// ========================================

// ----------------------------- 
// -=[ MENU & BUTTON SECTION ]=-
// -----------------------------
// Define Struct for Menu
typedef struct
 {
     String title;
     byte type;
     // 0 - Back to Up Menu
     // 1 - Yes/No
     // 2 - Clock Format
     // 3 - Has SubMenu Standard Type
     // 4 - Digit Format
     byte value; // Different for Types
     byte parent; // Parent Menu
     uint8_t* Icon;
 }  Menu_Struct;
// --------------------------

Menu_Struct MMenus[21]; // Number of ALL Items


volatile boolean MenuButton = false;
// Interrupt Menu-Button Function
// --------------------------
void MenuButtonPressed()
{
    MenuButton = true;
    LCDLight(true);
}
// --------------------------

// Get Button Status
// --------------------------
unsigned long DebounceTime;
byte PrevBut = 0;

void GetButtonStatus () {
  bool AnyKey = false;
  unsigned long CurrTime;  
  CurrTime = millis();
  if  ((digitalRead(BUT_UP) == LOW)&&((CurrTime - DebounceTime > 400)))    
      { ButtonStatus[1] = true; AnyKey = true; PrevBut = 1;} // UP
  if  ((digitalRead(BUT_DOWN) == LOW)&&((CurrTime - DebounceTime > 500)))  
      { ButtonStatus[2] = true; AnyKey = true; PrevBut = 2;} // DOWN
  if  (((digitalRead(BUT_OK) == HIGH)||((MenuButton)))&&((CurrTime - DebounceTime > 300))&&(PrevBut != 3)) 
      { ButtonStatus[3] = true; AnyKey = true; MenuButton = false; PrevBut = 3;} else { MenuButton = false; } // OK-MENU
  
  if (CurrTime - DebounceTime > 1500) {PrevBut = 0; } // Reset Press Button Memory
  ButtonStatus[0] = AnyKey; // Any Button Pressed
  
  if (AnyKey) { DebounceTime = millis(); }
  delay(30); // Delay for Debounce
}
// --------------------------

// Clear Status for ALL Button
// --------------------------
void ClearButtonStatus () {
  for (byte i = 0; i < (sizeof(ButtonStatus)/sizeof(bool)); i++) { ButtonStatus[i] = false; }
  MenuButton = false;
}
// --------------------------

// Get First Child for Parent
// --------------------------
byte GetFirstChild(byte numItem) {
  for (byte i = 0; i < (sizeof(MMenus)/sizeof(Menu_Struct)); i++)   
    {
      if (MMenus[i].parent == numItem) {return i;}
    }
    return 1;
}
// --------------------------

// Show T2 Elements
// --------------------------
void ShowT2Menu (byte numItem, byte act = 0) {
    byte xT2 = 0;
    byte cT2 = 0;  
    if (MMenus[numItem].type == 4) {numItem = MMenus[numItem].parent;}
    lcd.setFont(BigNumbers);
    xT2 = 0;
    cT2 = 0;
    for (byte i = 0; i < (sizeof(MMenus)/sizeof(Menu_Struct)); i++)   
      { // My Child & Type 4
        if ((MMenus[i].parent == numItem)&&(MMenus[i].type == 4)) {
            xT2++;
            if (MMenus[i].value < 10){ lcd.print("0" + String(MMenus[i].value), 10+cT2, 15);} 
                else { 
                  if ((MMenus[i].value < 20)&&(cT2 > 0)) {cT2 = cT2 - 7;}
                  lcd.print(String(MMenus[i].value), 10+cT2, 15);
                  }
            if (act == xT2) {
              if ((MMenus[i].value >= 10 )&&(MMenus[i].value < 20)&&(cT2 > 0)) lcd.drawRect(15+cT2, 12, 46+cT2, 42);
                else lcd.drawRect(8+cT2, 12, 39+cT2, 42);
              }
            cT2 = cT2 + 37;
            if (xT2 > 2) { break; } // Not more 2
            lcd.drawRect(42, 23, 43, 24); lcd.drawRect(42, 29, 43, 30);
        }
      }
}
// --------------------------

// Check Limit for Items Value
// --------------------------
byte CheckLimitItem (byte numItem) {
  if (MMenus[numItem].title=="Hours") return 23;
  if (MMenus[numItem].title=="Minutes") return 59;
}
// --------------------------

// Change Digital Value for Menu Item
// --------------------------
void ChangeDigital (byte numItem, byte act = 0) {
  // Change Screen
  lcd.clrScr();
  lcd.setFont(SmallFont);
  lcd.print("Set "+MMenus[numItem].title, CENTER, 0);
  ShowT2Menu(numItem, act);
  lcd.update();
  // while not press OK Button
  do {
    GetButtonStatus();
    if (ButtonStatus[0]) { // Any Key Pressed
        if (ButtonStatus[2]) { // Press DOWN
          if (MMenus[numItem].value == 0) {MMenus[numItem].value = CheckLimitItem(numItem);}
              else MMenus[numItem].value--;
        } else if (ButtonStatus[1]) { // Press UP
          if (MMenus[numItem].value == CheckLimitItem(numItem)) {MMenus[numItem].value = 0;} // Go To First Items of Parent
              else MMenus[numItem].value++; 
        } else if (ButtonStatus[3]) { ClearButtonStatus(); break; } // EXIT FROM LOOP
      lcd.clrScr();
      lcd.setFont(SmallFont);
      lcd.print("Set "+MMenus[numItem].title, CENTER, 0);
      ShowT2Menu(numItem, act);
      lcd.update();
    }
    ClearButtonStatus(); 
  } while(1); // Infinity Loop
}
// --------------------------

// Update all Settings from Menu Item Values
// --------------------------
void UpdateSettings(){
/*
// Init Link Data with Menu Item Value as Below
    0                  1           2                 3            4          
// CurHour [11], CurMin [12], AlarmHour [17], AlarmMin [18], AlarmActive [5],
        5              6                 7                8 
// MelodyAlarm [8], BellAlarm [9], WeatherMeasure [14], WeatherWarning[15]
const byte LinkData[] = {11, 12, 17, 18, 5, 8, 9, 14, 15};
*/
    // Current Time
    if ((MMenus[LinkData[0]].value != tm.peek(0))||(MMenus[LinkData[1]].value != tm.peek(1)))
        { tm.setTime (MMenus[LinkData[0]].value, MMenus[LinkData[1]].value, 0);}
    // All other tor RTC Memory
    for (byte i = 2; i < (sizeof(LinkData)/sizeof(byte)); i++) {
      if (MMenus[LinkData[i]].value != tm.peek(i)) { 
        if ((MMenus[LinkData[i]].type == 1)&&(MMenus[LinkData[i]].value > 1)) { MMenus[LinkData[i]].value = 0; }
        tm.poke(i, MMenus[LinkData[i]].value); 
        }
    }
}
// --------------------------
 
// Show Menu
// --------------------------
void ShowStructMenu (byte numItem) {
  lcd.clrScr();
  //LCDLight(true);
  UpdateSettings();
  byte x = 1; // Count Items for Current Parent
  byte xItem = 0; // Current Position
  do {
    lcd.setFont(SmallFont);
    ClearButtonStatus();
    if (xItem == 0) // Detect Current Item
    {
      lcd.clrScr();
      lcd.print(MMenus[numItem].title,CENTER,0); // Print Title Current Item
      // Loop for All Menu for Detect xItem and Count Parents Items
      for (byte i = 0; i < (sizeof(MMenus)/sizeof(Menu_Struct)); i++)   
        { // Parent as is
          if (MMenus[i].parent == MMenus[numItem].parent) {
              if (i == numItem) { xItem = x; } // Number Item as Set
              x++; // Count Items for Parent
              }
        }
      x = x-1; // Correct Count

      // Show Current Item
      // 1 - show Title
      // 2 - Detect Type
      // 3 - Show Icon or Other Style
      // ~~~~~~~~~~~~~~~~~~~~~~
      switch (MMenus[numItem].type) {
       case 0: // 0 - Exit Items
              lcd.drawBitmap(28, 13, IconExit, 32, 32);
              break;
       case 1: // 1 - Checked Items
              if (MMenus[numItem].value == 1) {lcd.drawBitmap(27, 13, IconOn, 32, 32); }
                                         else {lcd.drawBitmap(27, 13, IconOff, 32, 32);}
              break;
       case 2: // 2 - Show Current value
              if (MMenus[numItem].parent == 0) {lcd.drawBitmap(28, 13, MMenus[numItem].Icon, 32, 32);}
                  else ShowT2Menu(numItem);
              break;
       case 3: // 3 - has SubMenu
              lcd.drawBitmap(28, 13, MMenus[numItem].Icon, 32, 32);
              break;
       case 4: // 4 - Show Change HH/MM
              ShowT2Menu(numItem, xItem);
              break;
      /* default: // For Others type show Debug info
              lcd.printNumI(numItem,10,10); 
              lcd.printNumI(MMenus[numItem].value,10,20); 
              lcd.printNumI(xItem,10,30); 
              lcd.printNumI(MMenus[numItem].type,10,40);*/
        }
      lcd.update(); // Send Info to LCD
      // ~~~~~~~~~~~~~~~~~~~~~~
    } // .end Detect Current Item
    
  // Check Button Status
  // ~~~~~~~~~~~~~~~~~~~~~~
  GetButtonStatus();
  if (ButtonStatus[0]) { // Any Key Pressed
    if (ButtonStatus[2]) { // Press DOWN
        if (xItem == 1) {numItem = numItem+x-1;} // Go To End Items of Parent
            else numItem--;
      } else if (ButtonStatus[1]) { // Press UP
                if (xItem == x) {numItem = numItem-x+1;} // Go To First Items of Parent
                    else numItem++; 
      } else if ((ButtonStatus[3])&&(MMenus[numItem].type == 0)) { // Press OK for Return Type Item
            // No parent and Return Type with OK-Button
            UpdateSettings();
            if (MMenus[numItem].parent == 0) {ClearButtonStatus(); break; } // EXIT FROM LOOP
                else {numItem = MMenus[numItem].parent;} // Go To Parent Menu
        // Press OK for SubMenu Type Item (2 and 3)
      } else if ((ButtonStatus[3])&&(MMenus[numItem].type == 3)||(MMenus[numItem].type == 2)) {
            numItem = GetFirstChild(numItem); // Go To Parent Menu
      } else if ((ButtonStatus[3])&&(MMenus[numItem].type == 1)) { MMenus[numItem].value = 1 - MMenus[numItem].value; // Invert Check
      } else if ((ButtonStatus[3])&&(MMenus[numItem].type == 4)) { ChangeDigital (numItem, xItem); // Start Change Value Function
      } // .end Button Status Check
      
    // Clear Button State and Menu Position
    ClearButtonStatus();  
    xItem = 0; x = 1;
    }
  // ~~~~~~~~~~~~~~~~~~~~~~
    // Check numItem for Error
    if ((numItem < 0)|| (numItem > (sizeof(MMenus)/sizeof(Menu_Struct)))) numItem = 0;  
  } while(1); // Infinity Loop for Menu
  ClearButtonStatus(); // Clear Current Press Button Status
  delay(1000);
}
// --------------------------

// Pause Function
// -----------------
void KeepCalm (word pp)
{
  delay(pp);
  GetButtonStatus();
  if (ButtonStatus[3]) {ShowStructMenu (1);}
  ClearButtonStatus();
}
// -----------------
// ========================================

// -----------------
void playMusic()
{
  // all below are music
  // iterate over the notes of the melody:
  for (byte thisNote = 0; thisNote < 8; thisNote++) {

    // to calculate the note duration, take one second
    // divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(BUZZ_PIN, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    GetButtonStatus();
  }
noTone(BUZZ_PIN);
}
// ========================================

// Show Current Time
// -----------------
void ShowClock() {
  bool AlarmOn = false;
  unsigned long AlarmTime = 0;
  bool MotorTurn = false;
    do
    {
    lcd.clrScr();
    // Update Current Time
    rtc_time = tm.getTime();
    MMenus[LinkData[0]].value = rtc_time.hour;
    MMenus[LinkData[1]].value = rtc_time.min;
    // Save to RTC Module Memory
    if (MMenus[LinkData[0]].value != tm.peek(0)) { tm.poke(0, MMenus[LinkData[0]].value); }
    if (MMenus[LinkData[1]].value != tm.peek(1)) { tm.poke(1, MMenus[LinkData[1]].value); }
    
    // Alarm Active
    if (MMenus[LinkData[4]].value == 1)
      {
        // Alarm Action
        if (((MMenus[LinkData[0]].value*60+MMenus[LinkData[1]].value) >= (MMenus[LinkData[2]].value*60+MMenus[LinkData[3]].value))&&
            ((MMenus[LinkData[0]].value*60+MMenus[LinkData[1]].value - MMenus[LinkData[2]].value*60-MMenus[LinkData[3]].value) <= 3))
            {
             if ((!AlarmOn)||(AlarmTime == 0))
             {
             // Speaker Action
             if (MMenus[LinkData[5]].value == 1) {LCDLight (false); playMusic(); }
             // Motor Action
             if (MMenus[LinkData[6]].value == 1) { analogWrite(MOTOR_PIN, motorSpeed); }
             AlarmTime = millis();
             MotorTurn = true;
             }
             LCDLight (!backlight);
             AlarmOn = true;
            } 
            else if (AlarmOn)
              {
                AlarmOn = false;
                if (MMenus[LinkData[6]].value == 1) { analogWrite(MOTOR_PIN, 0);}
                //if (MMenus[LinkData[5]].value == 1) { noTone(BUZZ_PIN); }
              }

        lcd.setFont(SmallFont);
        if (!AlarmOn)
        {
          if ((MMenus[LinkData[5]].value == 1)||(MMenus[LinkData[6]].value == 1)) { lcd.drawBitmap(0, 0, IconAlarmMini, 8, 8); }
          if (MMenus[LinkData[2]].value < 10){ lcd.print("0" + String(MMenus[LinkData[2]].value)+":", 10, 1); }
              else lcd.print(String(MMenus[LinkData[2]].value)+":", 10, 1);
          if (MMenus[LinkData[3]].value < 10){ lcd.print("0" + String(MMenus[LinkData[3]].value), 27, 1); }
              else lcd.print(String(MMenus[LinkData[3]].value), 27, 1);
        } else {lcd.print("!!!WAKE UP!!!", CENTER, 0);}
      }

    lcd.setFont(BigNumbers);

    // ==========[time]
    if (rtc_time.hour < 10){
      lcd.print("0" + String(rtc_time.hour), 10, 12);
    }
    else {
      lcd.print(String(rtc_time.hour), 10, 12);
    }
    if (rtc_time.min < 10){
      lcd.print("0" + String(rtc_time.min), 47, 12);
    }
    else if (rtc_time.min < 20) {
      lcd.print(String(rtc_time.min), 40, 12);
    }
    else {
      lcd.print(String(rtc_time.min), 47, 12);
    }
    // Show Date
    if (MMenus[LinkData[9]].value == 1){
      lcd.setFont(SmallFont);
      lcd.print(tm.getDateStr(FORMAT_LONG,FORMAT_LITTLEENDIAN,'/'), CENTER, 41);
    }
    
    if (!AlarmOn) {
    for (byte i=0; i < 5; i++){
      lcd.clrRect(42, 20, 43, 27);
      lcd.update();
      delay(500);
      lcd.drawRect(42, 20, 43, 21);
      lcd.drawRect(42, 26, 43, 27);
      lcd.update();
      delay(500);
    }} else {
      lcd.drawRect(42, 20, 43, 21);
      lcd.drawRect(42, 26, 43, 27);
      lcd.update();
    }

    // Check Alarm Action State
    if (AlarmOn) {
      if (millis() - AlarmTime > (4000*(MMenus[LinkData[6]].value+1)))
        {
          if (MotorTurn) { 
            MotorTurn = false; 
            if (MMenus[LinkData[6]].value == 1) { analogWrite(MOTOR_PIN, 0);}
            //if (MMenus[LinkData[5]].value == 1) { noTone(BUZZ_PIN); }
          }
          if (millis() - AlarmTime > (7500*(MMenus[LinkData[6]].value+1))) {AlarmTime = 0;}
        }
      
      GetButtonStatus();
      if (ButtonStatus[0]) {
          if (MMenus[LinkData[6]].value == 1) { analogWrite(MOTOR_PIN, 0);}
          //if (MMenus[LinkData[5]].value == 1) { noTone(BUZZ_PIN); }
          LCDLight (false); // LCD Light On
          MMenus[LinkData[4]].value = 0; // Change Item Value 
          UpdateSettings(); // Save New Settings on RTC Module
          AlarmOn = false;
          delay(1000);
          }
      ClearButtonStatus();
    }
   } while (AlarmOn);
}
// ========================================

// Calculate Normal Pressure for Altitude & Temperature
// -----------------
float GetNormalPressure(int a, int t) {
  float L = 0.0065;
  float B;
  B = L / (273.15+t);
  return (1013.25*pow((1-B*a),5.2556));
}
// ========================================

// BME Get & Store Data from Sensor
// -----------------
float BMEGetData()
{
  temperature = bme.readTemperature();
  humidity = bme.readHumidity();
  pressure = bme.readPressure();
  pressure = round(pressure/100.0F);

  // Calculate Pressure
  if (my_forecast[0] == 0)
    { // First Start or After Update Settings
      for (byte i = 0; i < 4; i++) { my_forecast[i] = pressure; }
      my_forecast[1] = 0; // Difference 
    } else {
      // Check Change
      if (my_forecast[0] > pressure) my_forecast[4] = 1; // Up
        else if (my_forecast[0] < pressure) my_forecast[4] = 2; // Down
          else my_forecast[4] = 0; // Equal
      my_forecast[0] = pressure; // Current
      // Fix maximum and minimum pressure
      if (forecast_count <= 15) // 3 Hours (every 12min)
      {
        if (my_forecast[0] > my_forecast[2]) my_forecast[2] = my_forecast[0];
        if (my_forecast[0] < my_forecast[3]) my_forecast[3] = my_forecast[0];
        my_forecast[1] = my_forecast[2] - my_forecast[3];
      } else {
       // Save new limit
       my_forecast[2] = my_forecast[0];
       my_forecast[3] = my_forecast[0] - (my_forecast[1] / 3);
       forecast_count = 0;
      }
    }
  forecast_count++;
}
// ========================================

// Celcius Icon
// -----------------
void drawCelcius(byte x, byte y){
  lcd.setFont(BigNumbers);
  lcd.drawCircle(x, y, 2);
  if (MMenus[LinkData[7]].value == 0) {
    lcd.printNumI(6,x+3,y);
    lcd.clrRect(x+5, y+22, x+15, y+23);
    lcd.clrRect(x+7, y+21, x+15, y+21);
    lcd.clrRect(x+14,y+13, x+15, y+23);
    lcd.clrRect(x+13,y+15, x+14, y+23);
  } else {
    lcd.printNumI(0,x+3,y);
    lcd.clrRect(x+14,y, x+15,y+24);
    lcd.clrRect(x+13,y+3, x+14,y+21);
  } 
}
// ========================================


// Show Percent
// -----------------
void drawPercent(byte x, byte y){
  lcd.drawCircle(x, y, 3);
  lcd.drawLine(x-2, y+19, x+8, y-3);
  lcd.drawCircle(x+7, y+16, 3);
}
// ========================================

// Draw Weather & Forecast
// -----------------
void drawWeather(){
  byte sel = 1;
  lcd.clrScr();
  float norma;
  lcd.setFont(SmallFont);
  
  norma = GetNormalPressure(altitude, temperature-5); // Get Norma

  // Weather Forecasting 
  if ((my_forecast[0] > norma+10) && (my_forecast[1] < 2)&&(my_forecast[4] <= 1)) {sel = 1;}
  //if  (my_forecast[0] < norma-5) {sel = 3;}
  else if ((my_forecast[0] < norma+15) && (my_forecast[1] > 2)) {sel = 2;}
  else if ((my_forecast[0] > (norma-30)) && (my_forecast[0] < (norma-3)) && (my_forecast[4] == 2)) {sel = 2;}
  else sel = 3;
  
  if ((old_sel != sel ) && (MMenus[LinkData[8]].value == 1)) {playMusic();}
  old_sel = sel;
  
  // Select Icon
  switch (sel) {
    case 1: // Clear
      lcd.drawBitmap(28, 13, IconClear, 24, 24);
      lcd.print("Sunny Weather",CENTER, 0);    
      break;
    case 2: // Rainy
      lcd.drawBitmap(28, 13, IconRainy, 24, 24);
      lcd.print("Rainy Weather",CENTER, 0);
      break;
    default: 
      lcd.drawBitmap(28, 13, IconCloudy, 24, 24);
      lcd.print("Cloudy Weather",CENTER, 0);
  } // .end Switch Select

  
  // Print Pressure and Forecast
  /*for (byte i = 0; i < 4; i++)
    lcd.print(String(round(my_forecast[i]))+" hPa "+String(my_forecast[4]),CENTER,10+(10*i)); */
  
  if (MMenus[LinkData[7]].value == 0) lcd.print(String(my_forecast[0])+" hPa",CENTER,38); 
    else lcd.print(String(round(my_forecast[0]*0.7501))+" mmHg",CENTER,38); 
    
  lcd.update();
}
// ========================================

// show temperature screen
// -----------------
void drawTemperature (){
  lcd.clrScr();
  lcd.setFont(BigNumbers);
  if (MMenus[LinkData[7]].value == 0)
    { 
      if (((temperature*1.8)+32) > 100)
          lcd.printNumF(round((temperature*1.8)+32),0,23,10);
        else lcd.printNumF(round((temperature*1.8)+32),0,35,10);
    } else {
      lcd.printNumF(temperature,0,35,10);
    }
    lcd.drawBitmap(0, 0, IconTemperature, 28, 48);
    drawCelcius(66, 10);
  lcd.update();
}
// ========================================

// show humidity screen
// -----------------
void drawHumidity (){
  lcd.clrScr();
  lcd.drawBitmap(0, 0, IconHumidity, 30, 48);
  lcd.setFont(BigNumbers);
  lcd.printNumF(humidity,0,35,10);
  drawPercent(68, 14); // draw percent sign at these coordinates
  lcd.setFont(SmallFont);
  if (humidity < 30) lcd.print("Too dry",36,38);
      else if (humidity < 55) lcd.print("Ideal",40,38);
              else lcd.print("Too humid",30,38);
  lcd.update();
}
// ========================================


// Fill Battery Current Status
// -----------------
void fillBat(byte percent){
  if (percent <= 100 && percent > 0){
    percent = 100 - percent;
    percent = map(percent,0,100,1,26);
    lcd.clrRect(8,9,9,percent+9);
    lcd.clrRect(10,9,11,percent+9);
    lcd.clrRect(12,9,13,percent+9);
    lcd.clrRect(14,9,15,percent+9);
    lcd.clrRect(16,9,17,percent+9);
    lcd.clrRect(18,9,19,percent+9);
    lcd.clrRect(20,9,21,percent+9);
  }
}
// ========================================

// Voltage and Lighting Show
// -----------------
void drawVoltage (){
  byte voltagePercent = 0;
  float voltage = analogRead(VOLT_PIN) * (5.0 / 1023.0);
  
  lcd.clrScr();
  lcd.drawBitmap(4, 3, IconBattery, 21, 40);

  if (voltage > 4.2) voltage = 4.2;
  voltagePercent = map(round(voltage*10), 26, 42, 0, 100);

  fillBat(voltagePercent);
  
  lcd.setFont(SmallFont);

  // if % less than 10 - start from position 6
  if (voltagePercent < 10){
    lcd.print(String(voltagePercent) + "%", 8, 41);
  }
  else if (voltagePercent < 100) {
    lcd.print(String(voltagePercent) + "%", 5, 41);
  }
  else {
    lcd.print(String(voltagePercent) + "%", 1, 41);
  }
  lcd.print("Voltage:",34,5);
  lcd.printNumF(voltage,1,34,15);
  lcd.print("V",55,15);
  lcd.print("Light:",34,25);
  lcd.print(String(lightIntensity[1]),34,35);
  lcd.update();
}
// ========================================
