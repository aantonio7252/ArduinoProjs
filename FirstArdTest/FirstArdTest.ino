//https://adafruit.github.io/Adafruit_SSD1306/html/class_adafruit___s_s_d1306.html
//https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>
#ifdef __AVR__
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#endif
#include <Adafruit_AHTX0.h>
#include <RTClib.h>
#include <Adafruit_MCP23008.h>

// Pins
#define TP4056_STDBY 2
#define TP4056_CHRG  3
#define BATT_VOLTAGE_PIN A0
#define SCREEN_ADDRESS 0x3C
#define OLED_RESET     -1

// I2C Devices
Adafruit_AHTX0 aht;
RTC_DS3231 rtc;
Adafruit_MCP23008 mcp;

// Device presence flags, set in initHardware() and used by serial telemetry
static bool ahtFound = false;
static bool rtcFound = false;
static bool mcpFound = false;

//Static variables
static bool Fah = true;
static bool isFinishedPcbMode = false;
static float smoothedBatteryVoltage = -1.0;
static float smoothedHumidity = -1.0;

//Function Variables for given attachments
/*
Function isCharging() returns True if charging
Function isChargeComplete() returns True if fully charged
Function getBatteryVoltage() returns the voltage of the battery
Function readAHT20(float &temp, float &hum) returns the temperature and humidity
Function readRTC() returns the current time and date
Function readSwitch(uint8_t swNum) returns the state of the switch

Variables temp, hum, batteryVoltage are declared in the global scope
Variables RTC is declared in the global scope

*/

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define gridW 16
#define gridH 16

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES     10 // Number of snowflakes in the animation example

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16

enum SystemState {
  STATE_MAIN,
  STATE_SETTIME,
  STATE_SETTEMP,
  STATE_SLEEP,
  STATE_MENU,
};

static SystemState currentState = STATE_MAIN;

static int menuSelection = 0; // 0: Main, 1: Set Time, 2: Set Temp, 3: Sleep
static int tempSelection = 0; // 0: Fahrenheit, 1: Celsius

// RTC configuration variables
static int setTimeField = 0;  // 0: Hour, 1: Minute, 2: Day, 3: Month, 4: Year
static int editHour = 12;
static int editMinute = 0;
static int editDay = 1;
static int editMonth = 1;
static int editYear = 2026;

// Button mapping and state change tracking
#define BTN_UP_LEFT 0
#define BTN_DOWN_RIGHT 1
#define BTN_YES 2
#define BTN_CANCEL 3

static bool lastBtnState[4] = {false, false, false, false};

// Function declarations for compiler compatibility
void enterSleepMode();
void showSetTimeScreen();
void showSetTempScreen();
void showMenuScreen();
int getJustPressedButton();
void drawBitmap(const uint8_t *bitmap, int x, int y, int w, int h);

#ifdef __AVR__
// Disable watchdog timer early in startup (.init3) to prevent bootloader reset loops during code upload
void disable_wdt_at_boot(void) __attribute__((naked)) __attribute__((section(".init3")));
void disable_wdt_at_boot(void) {
  MCUSR = 0;
  wdt_disable();
}

ISR(WDT_vect) {
  // Watchdog timer interrupt (wakes up MCU from sleep)
}

void setupWDT() {
  cli();
  MCUSR &= ~(1 << WDRF);
  WDTCSR |= (1 << WDCE) | (1 << WDE);
  // Set watchdog interrupt mode and prescaler to 1.0 second
  WDTCSR = (1 << WDIE) | (1 << WDP2) | (1 << WDP1);
  sei();
}

void disableWDT() {
  cli();
  WDTCSR |= (1 << WDCE) | (1 << WDE);
  WDTCSR = 0x00;
  sei();
}
#endif

static const unsigned char PROGMEM logo_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

  static const unsigned char PROGMEM battery[] =
{ 
B01110,
B10001,
B10101,
B10101,
B10101,
B10101,
B10101,
B10001,
B11111};
static const unsigned char PROGMEM temp[] ={
B00100,
B01010,
B01010,
B01010,
B01010,
B01010,
B10001,
B10001,
B01110,
};

static const unsigned char PROGMEM hum[] =
{
B00100,
B00100,
B01110,
B01110,
B11111,
B11111,
B11111,
B01110,
};

static const unsigned char PROGMEM vivlos[] = {
  B00000000, B00000000, B00010000, B00000000, B00000000, B00000000,
  B00000000, B00000000, B00111000, B00000000, B00110000, B00000000,
  B00000000, B00000000, B00101100, B00000000, B01110000, B00000000,
  B00000000, B00000000, B00100100, B01111100, B11110000, B00000000,
  B00000000, B00000000, B00100110, B11001001, B11110000, B00000000,
  B00000000, B00000000, B00100001, B00011011, B11110000, B00000000,
  B00000000, B00000111, B00100010, B00011111, B11100000, B00000000,
  B00000000, B00001000, B10110100, B00001100, B11100000, B00000000,
  B00000000, B00001000, B01011000, B00111000, B01100000, B00000000,
  B00000000, B00011000, B00111000, B11100100, B00101111, B00000000,
  B00000000, B00010110, B01110001, B10000010, B00110111, B00000000,
  B00000000, B00010011, B11100111, B00000001, B11010011, B00000000,
  B00000000, B00010000, B11011100, B00000000, B00010001, B00000000,
  B00000000, B00011111, B11110000, B00000000, B00011001, B10000000,
  B00000000, B00010000, B10000000, B00000000, B00010100, B10000000,
  B00000000, B00011111, B10000001, B00000000, B00010010, B10000000,
  B00000000, B00010001, B10000111, B00001100, B00010001, B10000000,
  B00000000, B00010011, B10001000, B10010010, B00010001, B10000000,
  B00000000, B00011100, B10010110, B10100001, B00010001, B10000000,
  B00000000, B00010000, B10101010, B10101100, B10010010, B10000000,
  B00000000, B00010001, B10100010, B11001010, B10010100, B10000000,
  B00000000, B00011110, B10000000, B10001000, B10011000, B10000000,
  B00000000, B00110001, B10010000, B00000001, B00010000, B10000000,
  B00000000, B00100011, B10001000, B00000011, B00111000, B10000000,
  B00000000, B00101111, B01001001, B01010110, B01010110, B10000000,
  B00000000, B00111110, B01000100, B10100100, B11011001, B10000000,
  B00000000, B00100010, B01000110, B00001101, B10010100, B10000000,
  B00000000, B00110010, B01000011, B11111001, B00010111, B10000000,
  B00000000, B00101110, B11000010, B00011001, B10011001, B10000000,
  B00000000, B01111111, B10001110, B00010100, B10010010, B10000000,
  B00000000, B01110101, B00001010, B00010010, B01111100, B10000000,
  B00000000, B01001111, B00010001, B11100001, B10110100, B10000000,
  B00000000, B11010110, B00110000, B00000000, B11011010, B10000000,
  B00000000, B10101110, B01101001, B11100001, B11101001, B10000000,
  B00000001, B10001010, B11000111, B00110011, B01111100, B10000000,
  B00000001, B01011011, B10000011, B00011110, B01001011, B10000000,
  B00000011, B00111000, B01000000, B10001100, B10001100, B10000000,
  B00000010, B00111000, B01000001, B11011100, B10001101, B10000000,
  B00001110, B00101000, B01000001, B01010110, B10001110, B11000000,
  B00011100, B01101000, B01000001, B01110010, B10101010, B01000000,
  B00010010, B11001000, B01100001, B11100011, B11001011, B01000000,
  B00110011, B10001000, B01011000, B11000000, B01011001, B01100000,
  B01100010, B00001111, B11001111, B10000000, B01010101, B10110000,
  B01000100, B00001000, B10000000, B10000000, B01110100, B10010000,
  B10011000, B00001000, B10000000, B10000000, B10100110, B11001000,
  B00010000, B00010000, B10000000, B10000000, B10100110, B01111000
};

// --------------------------------------------------------
// Hardware Functions
// --------------------------------------------------------
void initHardware() { //Checks whether sensors exist
  if (!aht.begin()) {
    ahtFound = false;
    Serial.println(F("Could not find AHT20"));
  } else {
    ahtFound = true;
  }
  if (!rtc.begin()) {
    rtcFound = false;
    Serial.println(F("Could not find RTC DS3231"));
  } else {
    rtcFound = true;
  }
  
  // Check if MCP23008 is present
  if (mcp.begin()) {
    mcpFound = true;
    isFinishedPcbMode = true;
    Serial.println(F("MCP23008 detected: Entering Finished PCB Mode"));
    
    // Configure MCP23008 pins as inputs with pullups
    // GP0-3 are the buttons; GP4-7 are unconnected, pullups keep
    // the serial telemetry GPIO readings stable instead of floating.
    for (int i = 0; i < 8; i++) {
      mcp.pinMode(i, INPUT);
      mcp.pullUp(i, HIGH);
    }
    
    // Configure TP4056 pins on finished PCB
    pinMode(TP4056_STDBY, INPUT_PULLUP);
    pinMode(TP4056_CHRG, INPUT_PULLUP);
  } else {
    mcpFound = false;
    isFinishedPcbMode = false;
    Serial.println(F("MCP23008 not found: Entering Prototype Mode"));
  }
}

bool isCharging() { 
  if (isFinishedPcbMode) {
    return digitalRead(TP4056_CHRG) == LOW;
  }
  return false; 
}
bool isChargeComplete() {
  if (isFinishedPcbMode) {
    return digitalRead(TP4056_STDBY) == LOW;
  }
  return false;
}

float getBatteryVoltage() {
  int rawValue = analogRead(BATT_VOLTAGE_PIN); 
  // Assuming 5V reference and voltage divider that halves the voltage
  float currentVoltage = (rawValue / 1023.0) * 3.7 * 2.0; 
  if (smoothedBatteryVoltage < 0.0) {
    smoothedBatteryVoltage = currentVoltage;
  } else {
    // Exponential Moving Average filter (alpha = 0.05) to stabilize readings
    smoothedBatteryVoltage = 0.05 * currentVoltage + 0.95 * smoothedBatteryVoltage;
  }
  return smoothedBatteryVoltage; 
} //Returns batt voltage
int getBatteryPercent(){
  return (getBatteryVoltage() / 3.7) * 100;
}

void readAHT20(float &temp, float &hum) {
  sensors_event_t humidity_event, temp_event;
  aht.getEvent(&humidity_event, &temp_event);
  temp = temp_event.temperature;
  float rawHum = humidity_event.relative_humidity;
  if (smoothedHumidity < 0.0) {
    smoothedHumidity = rawHum;
  } else {
    // Exponential Moving Average filter (alpha = 0.1) to stabilize readings
    smoothedHumidity = 0.1 * rawHum + 0.9 * smoothedHumidity;
  }
  hum = smoothedHumidity;
} //variables: temp, hum

DateTime readRTC() {
  return rtc.now();
}

const uint8_t btnPins[4] = {2, 3, 4, 5};

bool readSwitch(uint8_t swNum) {
  if (swNum >= 4) return false;
  if (isFinishedPcbMode) {
    return mcp.digitalRead(swNum) == LOW;
  } else {
    return digitalRead(btnPins[swNum]) == LOW;
  }
}

// --------------------------------------------------------
// Serial Telemetry
// --------------------------------------------------------
static unsigned long lastSerialPrint = 0;
static const unsigned long SERIAL_INTERVAL_MS = 1000; // 1 line per second

void printPaddedNumber(int v) {
  if (v < 10) Serial.print('0');
  Serial.print(v);
}

void printSerialTelemetry() {
  // DS3231: date and time
  if (rtcFound) {
    DateTime now = readRTC();
    Serial.print(now.year());
    Serial.print('-');
    printPaddedNumber(now.month());
    Serial.print('-');
    printPaddedNumber(now.day());
    Serial.print(' ');
    printPaddedNumber(now.hour());
    Serial.print(':');
    printPaddedNumber(now.minute());
    Serial.print(':');
    printPaddedNumber(now.second());
  } else {
    Serial.print(F("RTC n/a"));
  }

  // AHT20: temperature (C and F) and humidity
  if (ahtFound) {
    float t = 0, h = 0;
    readAHT20(t, h);
    Serial.print(F(" | AHT "));
    Serial.print(t, 1);
    Serial.print('C');
    Serial.print('/');
    Serial.print(t * 9.0f / 5.0f + 32.0f, 1);
    Serial.print('F');
    Serial.print(' ');
    Serial.print(h, 1);
    Serial.print('%');
  } else {
    Serial.print(F(" | AHT n/a"));
  }

  // DS3231: internal temperature sensor (it has one, ~-40..+85C)
  if (rtcFound) {
    Serial.print(F(" | DS3231 "));
    Serial.print(rtc.getTemperature(), 1);
    Serial.print('C');
  }

  // MCP23008: all 8 GPIO states (1 = high / unpressed)
  if (mcpFound) {
    Serial.print(F(" | MCP "));
    for (int i = 0; i < 8; i++) {
      Serial.print(mcp.digitalRead(i) ? '1' : '0');
    }
  } else {
    Serial.print(F(" | MCP n/a"));
  }

  // Buttons: Up/Down/Yes/Cancel (MCP GP0-3 on finished PCB, D2-D5 in prototype)
  Serial.print(F(" | BTN U"));
  Serial.print(readSwitch(BTN_UP_LEFT) ? '1' : '0');
  Serial.print(F(" D"));
  Serial.print(readSwitch(BTN_DOWN_RIGHT) ? '1' : '0');
  Serial.print(F(" Y"));
  Serial.print(readSwitch(BTN_YES) ? '1' : '0');
  Serial.print(F(" C"));
  Serial.print(readSwitch(BTN_CANCEL) ? '1' : '0');

  // Battery voltage, percent, and TP4056 charging status
  Serial.print(F(" | Bat "));
  Serial.print(getBatteryVoltage(), 2);
  Serial.print('V');
  Serial.print(' ');
  Serial.print(getBatteryPercent());
  Serial.print('%');
  Serial.print(F(" Chg:"));
  Serial.print(isCharging() ? 'y' : 'n');
  Serial.print(F(" Full:"));
  Serial.print(isChargeComplete() ? 'y' : 'n');

  Serial.println();
}

void setup() {
  #ifdef __AVR__
  MCUSR = 0;
  wdt_disable();
  #endif

  Serial.begin(115200);
  delay(300);            // Startup grace period to stabilize Serial/DTR lines on code upload
  Wire.begin();          // Initialize I2C bus before talking to sensors
  // NOTE: Do NOT enable internal pull-ups (they pull to 5V).
  // Use external 4.7k resistors to 3.3V instead.
  initHardware();

  // Serial telemetry banner + immediate first reading
  Serial.println(F("--- FirstArdTest telemetry (1 line/sec) ---"));
  printSerialTelemetry();

  // In Prototype Mode, configure D2-D5 as button inputs
  if (!isFinishedPcbMode) {
    for (int i = 0; i < 4; i++) {
      pinMode(btnPins[i], INPUT_PULLUP);
    }
  }

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    //for(;;); // Don't proceed, loop forever
  }
  
  // Initialize button states
  for (int i = 0; i < 4; i++) {
    lastBtnState[i] = readSwitch(i);
  }

  // Show initial display buffer contents on the screen
  display.display(); //Shows the adafruit dispay
  delay(1000); // Pause for 2 seconds
  display.clearDisplay();
}

int getJustPressedButton() {
  int pressedBtn = -1;
  for (int i = 0; i < 4; i++) {
    // Buttons are connected to GP0-3 on the MCP23008.
    // readSwitch(i) returns true if GP(i) is LOW (pressed).
    bool currState = readSwitch(i); 
    if (currState && !lastBtnState[i]) {
      pressedBtn = i;
    }
    lastBtnState[i] = currState;
  }
  return pressedBtn;
}

void loop() {
  // Serial telemetry: one line per second (loop is blocked during sleep,
  // so nothing prints while asleep)
  unsigned long nowMs = millis();
  if (nowMs - lastSerialPrint >= SERIAL_INTERVAL_MS) {
    lastSerialPrint = nowMs;
    printSerialTelemetry();
  }

  int btn = getJustPressedButton();
  
  // State transitions and button logic
  switch (currentState) {
    case STATE_MAIN:
      if (btn == BTN_YES) {
        menuSelection = 0; // Reset menu selection
        currentState = STATE_MENU;
      } else if (btn == BTN_CANCEL) {
        currentState = STATE_SLEEP;
      }
      break;

    case STATE_MENU:
      if (btn == BTN_UP_LEFT) {
        menuSelection--;
        if (menuSelection < 0) menuSelection = 3;
      } else if (btn == BTN_DOWN_RIGHT) {
        menuSelection++;
        if (menuSelection > 3) menuSelection = 0;
      } else if (btn == BTN_YES) {
        if (menuSelection == 0) {
          currentState = STATE_MAIN;
        } else if (menuSelection == 1) {
          // Initialize edit variables with current RTC time
          DateTime now = readRTC();
          editHour = now.hour();
          editMinute = now.minute();
          editDay = now.day();
          editMonth = now.month();
          editYear = now.year();
          setTimeField = 0; // Start with Hour
          currentState = STATE_SETTIME;
        } else if (menuSelection == 2) {
          // Initialize select state from current Fah setting
          tempSelection = Fah ? 0 : 1;
          currentState = STATE_SETTEMP;
        } else if (menuSelection == 3) {
          currentState = STATE_SLEEP;
        }
      } else if (btn == BTN_CANCEL) {
        currentState = STATE_MAIN;
      }
      break;

    case STATE_SETTIME:
      if (btn == BTN_CANCEL) {
        currentState = STATE_MENU;
      } else if (btn == BTN_YES) {
        setTimeField++;
        if (setTimeField > 4) {
          // Save edited time to RTC
          rtc.adjust(DateTime(editYear, editMonth, editDay, editHour, editMinute, 0));
          currentState = STATE_MENU;
        }
      } else if (btn == BTN_UP_LEFT) {
        // Decrement active field
        if (setTimeField == 0) { // Hour (0-23)
          editHour = (editHour + 23) % 24;
        } else if (setTimeField == 1) { // Minute (0-59)
          editMinute = (editMinute + 59) % 60;
        } else if (setTimeField == 2) { // Day (1-31)
          editDay--;
          if (editDay < 1) editDay = 31;
        } else if (setTimeField == 3) { // Month (1-12)
          editMonth--;
          if (editMonth < 1) editMonth = 12;
        } else if (setTimeField == 4) { // Year (2000-2099)
          editYear--;
          if (editYear < 2000) editYear = 2099;
        }
      } else if (btn == BTN_DOWN_RIGHT) {
        // Increment active field
        if (setTimeField == 0) { // Hour
          editHour = (editHour + 1) % 24;
        } else if (setTimeField == 1) { // Minute
          editMinute = (editMinute + 1) % 60;
        } else if (setTimeField == 2) { // Day
          editDay++;
          if (editDay > 31) editDay = 1;
        } else if (setTimeField == 3) { // Month
          editMonth++;
          if (editMonth > 12) editMonth = 1;
        } else if (setTimeField == 4) { // Year
          editYear++;
          if (editYear > 2099) editYear = 2000;
        }
      }
      break;

    case STATE_SETTEMP:
      if (btn == BTN_UP_LEFT || btn == BTN_DOWN_RIGHT) {
        tempSelection = (tempSelection == 0) ? 1 : 0;
      } else if (btn == BTN_YES) {
        Fah = (tempSelection == 0);
        currentState = STATE_MENU;
      } else if (btn == BTN_CANCEL) {
        currentState = STATE_MENU;
      }
      break;

    case STATE_SLEEP:
      // Sleep state entry is handled by enterSleepMode()
      break;
  }

  // Draw UI based on state
  switch (currentState) {
    case STATE_MAIN:
      tehuba();
      break;
    case STATE_MENU:
      showMenuScreen();
      break;
    case STATE_SETTIME:
      showSetTimeScreen();
      break;
    case STATE_SETTEMP:
      showSetTempScreen();
      break;
    case STATE_SLEEP:
      enterSleepMode();
      break;
  }

  display.display();
  delay(50); // Small delay for polling responsiveness and debouncing
  display.clearDisplay();
}

void tehuba() {
  int slide = 0;
  int icon = 7;
  float t = 10, h = 10;
  readAHT20(t, h);
  t = ceil(t); h = ceil(h);
  DateTime now = readRTC();
  int battPct = getBatteryPercent();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  slide = 8;

  drawBitmap(temp, 2, 0, 8, 9); 
  display.setCursor(slide + 4, 0);
  if (Fah) {
    display.print((int)(t * 9 / 5 + 32));
    display.println("F");
  } else {
    display.print((int)t);
    display.println("C");
  }

  slide = 48;
  display.setCursor(slide + 10, 0);
  drawBitmap(hum, slide, 0, 8, 9);
  display.print((int)h);
  display.println(F("%"));

  slide = 88;
  display.setCursor(slide + 10, 0);
  drawBitmap(battery, slide, 0, 8, 9); 
  display.print(battPct); 
  display.println(F("%"));

  // Bouncing logic for the centered vivlos bitmap (45x40 pixels)
  int yOffset = 0;
  unsigned long cycleTime = millis() % 6000; // 6-second cycle: bounce for 1.5s, rest for 4.5s
  if (cycleTime < 1500) {
    // 2 bounces in 1.5 seconds (frequency = 1.33 Hz)
    float angle = (cycleTime / 1500.0) * 2.0 * PI;
    yOffset = -abs(sin(angle)) * 5; // bounce height of 5 pixels
  }

  // Draw the vivlos bitmap in the center of the screen
  // Centered horizontally: (128 - 45) / 2 = 41
  // Centered vertically (cut off bottom to 40px height): (64 - 40) / 2 = 12
  drawBitmap(vivlos, 41, 12 + yOffset, 45, 40);

  // Draw the time on the bottom row of the screen
  if (rtcFound) {
    display.setTextSize(1);
    display.setCursor(40, 56);
    if (now.hour() < 10) display.print('0');
    display.print(now.hour());
    display.print(':');
    if (now.minute() < 10) display.print('0');
    display.print(now.minute());
    display.print(':');
    if (now.second() < 10) display.print('0');
    display.print(now.second());
  } else {
    display.setTextSize(1);
    display.setCursor(40, 56);
    display.print(F("RTC n/a"));
  }
}

void showMenuScreen() {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(F("------ MENU ------"));
  display.println();

  display.print(menuSelection == 0 ? F("> ") : F("  "));
  display.println(F("1. Main Display"));

  display.print(menuSelection == 1 ? F("> ") : F("  "));
  display.println(F("2. Set Time"));

  display.print(menuSelection == 2 ? F("> ") : F("  "));
  display.println(F("3. Configure Temp"));

  display.print(menuSelection == 3 ? F("> ") : F("  "));
  display.println(F("4. Sleep Mode"));
}

void showSetTimeScreen() {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(F("--- SET TIME ---"));
  display.println();

  display.print(F("Time: "));
  if (setTimeField == 0) display.print(F("["));
  if (editHour < 10) display.print('0');
  display.print(editHour);
  if (setTimeField == 0) display.print(F("]"));
  
  display.print(F(":"));
  
  if (setTimeField == 1) display.print(F("["));
  if (editMinute < 10) display.print('0');
  display.print(editMinute);
  if (setTimeField == 1) display.print(F("]"));

  display.println();
  display.print(F("Date: "));
  
  if (setTimeField == 3) display.print(F("["));
  display.print(editMonth);
  if (setTimeField == 3) display.print(F("]"));
  
  display.print(F("/"));
  
  if (setTimeField == 2) display.print(F("["));
  display.print(editDay);
  if (setTimeField == 2) display.print(F("]"));
  
  display.print(F("/"));
  
  if (setTimeField == 4) display.print(F("["));
  display.print(editYear);
  if (setTimeField == 4) display.print(F("]"));

  display.println();
  display.println();
  display.println(F("U/D:Adj  Y:Next  C:Esc"));
}

void showSetTempScreen() {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(F("--- SET TEMP UNIT ---"));
  display.println();

  display.print(tempSelection == 0 ? F("> ") : F("  "));
  display.print(F("["));
  display.print(Fah ? F("*") : F(" "));
  display.println(F("] Fahrenheit"));
  
  display.print(tempSelection == 1 ? F("> ") : F("  "));
  display.print(F("["));
  display.print(!Fah ? F("*") : F(" "));
  display.println(F("] Celsius"));
  
  display.println();
  display.println(F("U/D:Move Y:Set C:Esc"));
}

void enterSleepMode() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(F("Entering Sleep..."));
  display.println(F("Press any button"));
  display.println(F("to wake up."));
  display.display();
  delay(1500);
  
  display.clearDisplay();
  display.display();
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  
  #ifdef __AVR__
  setupWDT();
  
  byte old_ADCSRA = ADCSRA;
  ADCSRA = 0; // Disable ADC
  
  bool buttonPressed = false;
  while (!buttonPressed) {
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    
    #if defined(BODS) && defined(BODSE)
    MCUCR = MCUCR | _BV(BODS) | _BV(BODSE);
    MCUCR = MCUCR & ~_BV(BODSE);
    #endif
    
    sleep_cpu(); // Sleep here
    
    sleep_disable();
    
    // Check if Serial is receiving data (e.g. code upload attempt or serial monitor)
    if (Serial.available()) {
      buttonPressed = true;
      break;
    }
    
    // Check if any button on MCP23008 is pressed (LOW)
    for (int i = 0; i < 4; i++) {
      if (readSwitch(i)) {
        buttonPressed = true;
        break;
      }
    }
  }
  
  disableWDT();
  ADCSRA = old_ADCSRA; // Restore ADC
  #else
  // Non-AVR fallback: wait for button press or Serial upload activity
  while (true) {
    if (Serial.available()) break;
    bool pressed = false;
    for (int i = 0; i < 4; i++) {
      if (readSwitch(i)) {
        pressed = true;
        break;
      }
    }
    if (pressed) break;
    delay(100);
  }
  #endif
  
  display.ssd1306_command(SSD1306_DISPLAYON);
  
  // Reinitialize button states to prevent waking up directly triggering an action
  for (int i = 0; i < 4; i++) {
    lastBtnState[i] = readSwitch(i);
  }
  
  currentState = STATE_MAIN;
}

void drawBitmap(const uint8_t *bitmap, int x, int y, int w, int h) {
  display.drawBitmap(x, y, bitmap, w, h, WHITE);
}
