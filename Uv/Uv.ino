#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // LCD 16x2

// Relay pins
const int relayWhitePin  = 3;
const int relayYellowPin = 4;
const int relayVioletPin = 5;
const int relayMotorPin  = 12; // Fan relay

// Potentiometer for relay selection
const int potPin = A0;

// TMP36 Temperature Sensor
const int tempPin = A1;
float temperatureC = 0;
const float tempThreshold = 100.0; // AUTO FAN threshold

// Main system switch
const int macroSwitch = 22;

// Fan mode buttons
const int fanForceOnButton  = 23; // always ON
const int fanForceOffButton = 24; // always OFF

// Fan mode states
int fanMode = 0; 
// 0 = AUTO (default)
// 1 = FORCE ON
// 2 = FORCE OFF

// Timer buttons
const int addHoursButton   = 6;
const int minusHoursButton = 7;
const int addMinutesButton = 8;
const int minusMinutesButton = 9;
const int addSecondsButton = 10;
const int minusSecondsButton = 11;

// Timer variables
unsigned long previousMillis = 0;
int hours = 2;
int minutes = 1;
int seconds = 0;

// Relay selector
int activeZone = -1;
const int zoneSize = 1024 / 3;
int selectedRelay = relayWhitePin;

// Debounce for fan buttons
unsigned long lastFanPress = 0;
const int fanDebounce = 200;

// -------------------------------------------------------
// SETUP
// -------------------------------------------------------
void setup() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Welcome to UV");
  delay(1000);
  lcd.clear();

  // Relay outputs
  pinMode(relayWhitePin, OUTPUT);
  pinMode(relayYellowPin, OUTPUT);
  pinMode(relayVioletPin, OUTPUT);
  pinMode(relayMotorPin, OUTPUT);

  // Switches
  pinMode(macroSwitch, INPUT_PULLUP);
  pinMode(fanForceOnButton, INPUT_PULLUP);
  pinMode(fanForceOffButton, INPUT_PULLUP);

  // Timer buttons
  pinMode(addHoursButton, INPUT_PULLUP);
  pinMode(minusHoursButton, INPUT_PULLUP);
  pinMode(addMinutesButton, INPUT_PULLUP);
  pinMode(minusMinutesButton, INPUT_PULLUP);
  pinMode(addSecondsButton, INPUT_PULLUP);
  pinMode(minusSecondsButton, INPUT_PULLUP);

  displayTime();
}

// -------------------------------------------------------
// MAIN LOOP
// -------------------------------------------------------
void loop() {
  int switchState = digitalRead(macroSwitch);

  // Update relay selector
  selectRelay();

  if (switchState == HIGH) {
    // SETUP MODE
    fanMode = 0; // reset fan to AUTO when in setup mode

    handleButtons();
    displayTime();

    // turn OFF everything in setup
    digitalWrite(relayWhitePin, HIGH);
    digitalWrite(relayYellowPin, HIGH);
    digitalWrite(relayVioletPin, HIGH);
    digitalWrite(relayMotorPin, HIGH);
  } 
  else {
    // RUN MODE
    runTimer();
    readTemperature();
    updateFanMode();
    controlFan();
  }
}

// -------------------------------------------------------
// FAN MODE CONTROL (AUTO / FORCE ON / FORCE OFF)
// -------------------------------------------------------
void updateFanMode() {
  if (digitalRead(fanForceOnButton) == LOW && millis() - lastFanPress > fanDebounce) {
    fanMode = 1;
    lastFanPress = millis();
  }

  if (digitalRead(fanForceOffButton) == LOW && millis() - lastFanPress > fanDebounce) {
    fanMode = 2;
    lastFanPress = millis();
  }
}

void controlFan() {
  if (fanMode == 1) {
    // FORCE ON
    digitalWrite(relayMotorPin, LOW);
  }
  else if (fanMode == 2) {
    // FORCE OFF
    digitalWrite(relayMotorPin, HIGH);
  }
  else {
    // AUTO MODE (temperature)
    if (temperatureC >= tempThreshold)
      digitalWrite(relayMotorPin, LOW);
    else
      digitalWrite(relayMotorPin, HIGH);
  }
}

// -------------------------------------------------------
// TIMER BUTTON HANDLING
// -------------------------------------------------------
void handleButtons() {
  if (digitalRead(addHoursButton) == LOW) { hours++; delay(150); }
  if (digitalRead(minusHoursButton) == LOW && hours > 0) { hours--; delay(150); }

  if (digitalRead(addMinutesButton) == LOW) {
    minutes++;
    if (minutes >= 60) { minutes = 0; hours++; }
    delay(150);
  }
  if (digitalRead(minusMinutesButton) == LOW && minutes > 0) { minutes--; delay(150); }

  if (digitalRead(addSecondsButton) == LOW) {
    seconds++;
    if (seconds >= 60) { seconds = 0; minutes++; }
    delay(150);
  }
  if (digitalRead(minusSecondsButton) == LOW && seconds > 0) { seconds--; delay(150); }
}

// -------------------------------------------------------
// TIMER COUNTDOWN
// -------------------------------------------------------
void runTimer() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= 1000) {
    previousMillis = currentMillis;

    if (hours > 0 || minutes > 0 || seconds > 0) {
      if (seconds == 0) {
        seconds = 59;
        if (minutes == 0) {
          minutes = 59;
          if (hours > 0) hours--;
        } else {
          minutes--;
        }
      } else {
        seconds--;
      }

      digitalWrite(relayWhitePin, HIGH);
      digitalWrite(relayYellowPin, HIGH);
      digitalWrite(relayVioletPin, HIGH);
      digitalWrite(selectedRelay, LOW);

      displayTime();
    } else {
      lcd.clear();
      lcd.print("TIMER DONE!");
      digitalWrite(relayWhitePin, LOW);
      digitalWrite(relayYellowPin, LOW);
      digitalWrite(relayVioletPin, LOW);
    }
  }
}

// -------------------------------------------------------
// DISPLAY UPDATE
// -------------------------------------------------------
void displayTime() {
  lcd.setCursor(0,0);
  lcd.print("T: ");
  if (hours < 10) lcd.print("0");
  lcd.print(hours); lcd.print(":");
  if (minutes < 10) lcd.print("0");
  lcd.print(minutes); lcd.print(":");
  if (seconds < 10) lcd.print("0");
  lcd.print(seconds);

  lcd.setCursor(0,1);
  lcd.print("R:");
  if (selectedRelay == relayWhitePin) lcd.print("W ");
  else if (selectedRelay == relayYellowPin) lcd.print("Y ");
  else lcd.print("V ");

  lcd.print("F:");
  if (fanMode == 1) lcd.print("ON ");
  else if (fanMode == 2) lcd.print("OFF");
  else lcd.print("AUTO");

  lcd.print(" ");
  lcd.print((int)temperatureC);
  lcd.print((char)223);
  lcd.print("C");
}

// -------------------------------------------------------
// POTENTIOMETER RELAY SELECTOR
// -------------------------------------------------------
void selectRelay() {
  int potValue = analogRead(potPin);
  int zone = potValue / zoneSize;

  if (zone != activeZone) {
    activeZone = zone;
    if (zone == 0) selectedRelay = relayWhitePin;
    else if (zone == 1) selectedRelay = relayYellowPin;
    else selectedRelay = relayVioletPin;

    displayTime();
  }
}

// -------------------------------------------------------
// READ TEMPERATURE
// -------------------------------------------------------
void readTemperature() {
  int rawValue = analogRead(tempPin);
  float voltage = rawValue * 5.0 / 1024.0;
  temperatureC = (voltage - 0.5) * 100.0;
}
