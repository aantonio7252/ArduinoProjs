//https://adafruit.github.io/Adafruit_SSD1306/html/class_adafruit___s_s_d1306.html
//https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Added for I2C Sensors
#include <Adafruit_AHTX0.h>
#include <RTClib.h>
#include <Adafruit_MCP23X08.h>

// Pins
#define TP4056_STDBY 2
#define TP4056_CHRG  3
#define BATT_VOLTAGE_PIN A0

// I2C Devices
Adafruit_AHTX0 aht;
RTC_DS3231 rtc;
Adafruit_MCP23X08 mcp;

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
{ B11111111, B11111000,
  B11111111, B11111100,
  B11000000, B00001111,
  B11000000, B00000111,
  B11000000, B00000111,
  B11000000, B00001111,
  B11111111, B11111100,
  B11111111, B11111000,};

// --------------------------------------------------------
// Hardware Functions
// --------------------------------------------------------
void initHardware() { //Checks whether sensors exist
  pinMode(TP4056_STDBY, INPUT_PULLUP);
  pinMode(TP4056_CHRG, INPUT_PULLUP);

  if (!aht.begin()) {
    Serial.println(F("Could not find AHT20"));
  }
  if (!rtc.begin()) {
    Serial.println(F("Could not find RTC DS3231"));
  }
  if (!mcp.begin_I2C()) {
    Serial.println(F("Could not find MCP23008"));
  } else {
    for (int i = 0; i < 4; i++) {
      mcp.pinMode(i, INPUT_PULLUP);
    }
  }
}

bool isCharging() { 
  return digitalRead(TP4056_CHRG) == LOW; }
bool isChargeComplete() {
  return digitalRead(TP4056_STDBY) == LOW;}

float getBatteryVoltage() {
  int rawValue = analogRead(BATT_VOLTAGE_PIN); 
  // Assuming 5V reference and voltage divider that halves the voltage
  return (rawValue / 1023.0) * 3.7 * 2.0; 
} //Returns batt voltage
int getBatteryPercent(){
  return (getBatteryVoltage() / 3.7) * 100;
}

void readAHT20(float &temp, float &hum) {
  sensors_event_t humidity_event, temp_event;
  aht.getEvent(&humidity_event, &temp_event);
  temp = temp_event.temperature;
  hum = humidity_event.relative_humidity;
} //variables: temp, hum

DateTime readRTC() {
  return rtc.now();
}

bool readSwitch(uint8_t swNum) {
  if (swNum >= 4) return false;
  return mcp.digitalRead(swNum) == LOW; 
}

void setup() {
  Serial.begin(115200);
  initHardware();

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen
  display.display(); //Shows the adafruit dispay
  delay(2000); // Pause for 2 seconds

  display.clearDisplay();
  display.display();
  delay(2000);


  //grid();
}
void placeBmap(const uint8_t *bitmap, int x, int y, int w, int h){
  drawBitmap(bitmap,x,y,w,h);
}
void drawBattery(int w, int h, int percent){
  //Battery is situated at top right corner, 16x16
  placeBmap(battery, display.width()-w, display.height()-h, w, h);
  for (int i=0;i<(percent * 9)/100;i++){
    display.drawRect(display.width()-w+4+i,display.height()-h)
  }
  display.display();
}
void grid(){
  int16_t i;
  display.clearDisplay(); // Clear display buffer
  display.drawRect(0,0,display.width(), display.height(), WHITE);
  for (int16_t i=0;i<display.height();i+=gridH){
    for (int16_t j=0;j<display.width();j+=gridW){
      display.drawPixel(j, i, WHITE);
      
    }
    display.display();
  }
}
void gridBounds(){
  int16_t i;
  display.clearDisplay(); // Clear display buffer

  for (int16_t i=0;i<(min(display.width(),display.height()))/2;i+=2){
    display.drawRect(i, i, display.width()-2*i, display.height()-2*i, WHITE);
    delay(10);
    display.display();
  }
  
}
void drawBitmap(const uint8_t *bitmap, int x, int y, int w, int h){
  display.drawBitmap(x, y, bitmap, w, h, WHITE);

}
void testdrawline() {
  int16_t i;

  display.clearDisplay(); // Clear display buffer

  for(i=0; i<display.width(); i+=4) {
    display.drawLine(0, 0, i, display.height()-1, WHITE);
    display.display(); // Update screen with each newly-drawn line
    delay(1);
  }
  for(i=0; i<display.height(); i+=4) {
    display.drawLine(0, 0, display.width()-1, i, WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();

  for(i=0; i<display.width(); i+=4) {
    display.drawLine(0, display.height()-1, i, 0, WHITE);
    display.display();
    delay(1);
  }
  for(i=display.height()-1; i>=0; i-=4) {
    display.drawLine(0, display.height()-1, display.width()-1, i, WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();

  for(i=display.width()-1; i>=0; i-=4) {
    display.drawLine(display.width()-1, display.height()-1, i, 0, WHITE);
    display.display();
    delay(1);
  }
  for(i=display.height()-1; i>=0; i-=4) {
    display.drawLine(display.width()-1, display.height()-1, 0, i, WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();

  for(i=0; i<display.height(); i+=4) {
    display.drawLine(display.width()-1, 0, 0, i, WHITE);
    display.display();
    delay(1);
  }
  for(i=0; i<display.width(); i+=4) {
    display.drawLine(display.width()-1, 0, i, display.height()-1, WHITE);
    display.display();
    delay(1);
  }

  delay(2000); // Pause for 2 seconds
}

void testdrawrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2; i+=2) {
    display.drawRect(i, i, display.width()-2*i, display.height()-2*i, WHITE);
    display.display(); // Update screen with each newly-drawn rectangle
    delay(1);
  }

  delay(2000);
}

void testfillrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2; i+=3) {
    // The INVERSE color is used so rectangles alternate white/black
    display.fillRect(i, i, display.width()-i*2, display.height()-i*2, INVERSE);
    display.display(); // Update screen with each newly-drawn rectangle
    delay(1);
  }

  delay(2000);
}

void testdrawcircle(void) {
  display.clearDisplay();

  for(int16_t i=0; i<max(display.width(),display.height())/2; i+=2) {
    display.drawCircle(display.width()/2, display.height()/2, i, WHITE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testfillcircle(void) {
  display.clearDisplay();

  for(int16_t i=max(display.width(),display.height())/2; i>0; i-=3) {
    // The INVERSE color is used so circles alternate white/black
    display.fillCircle(display.width() / 2, display.height() / 2, i, INVERSE);
    display.display(); // Update screen with each newly-drawn circle
    delay(1);
  }

  delay(2000);
}

void testdrawroundrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2-2; i+=2) {
    display.drawRoundRect(i, i, display.width()-2*i, display.height()-2*i,
      display.height()/4, WHITE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testfillroundrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2-2; i+=2) {
    // The INVERSE color is used so round-rects alternate white/black
    display.fillRoundRect(i, i, display.width()-2*i, display.height()-2*i,
      display.height()/4, INVERSE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testdrawtriangle(void) {
  display.clearDisplay();

  for(int16_t i=0; i<max(display.width(),display.height())/2; i+=5) {
    display.drawTriangle(
      display.width()/2  , display.height()/2-i,
      display.width()/2-i, display.height()/2+i,
      display.width()/2+i, display.height()/2+i, WHITE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testfilltriangle(void) {
  display.clearDisplay();

  for(int16_t i=max(display.width(),display.height())/2; i>0; i-=5) {
    // The INVERSE color is used so triangles alternate white/black
    display.fillTriangle(
      display.width()/2  , display.height()/2-i,
      display.width()/2-i, display.height()/2+i,
      display.width()/2+i, display.height()/2+i, INVERSE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testdrawchar(void) {
  display.clearDisplay();

  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  // Not all the characters will fit on the display. This is normal.
  // Library will draw what it can and the rest will be clipped.
  for(int16_t i=0; i<256; i++) {
    if(i == '\n') display.write(' ');
    else          display.write(i);
  }

  display.display();
  delay(2000);
}

void testdrawstyles(void) {
  display.clearDisplay();

  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(F("Hello, world!"));

  display.setTextColor(BLACK, WHITE); // Draw 'inverse' text
  display.println(3.141592);

  display.setTextSize(2);             // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.print(F("0x")); display.println(0xDEADBEEF, HEX);

  display.display();
  delay(2000);
}

void testscrolltext(void) {
  display.clearDisplay();

  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(10, 0);
  display.println(F("scroll"));
  display.display();      // Show initial text
  delay(100);

  // Scroll in various directions, pausing in-between:
  display.startscrollright(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrollleft(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrolldiagright(0x00, 0x07);
  delay(2000);
  display.startscrolldiagleft(0x00, 0x07);
  delay(2000);
  display.stopscroll();
  delay(1000);
}

void testdrawbitmap(void) {
  display.clearDisplay();

  display.drawBitmap(
    (display.width()  - LOGO_WIDTH ) / 2,
    (display.height() - LOGO_HEIGHT) / 2,
    logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, 1);
  display.display();
  delay(1000);
}

#define XPOS   0 // Indexes into the 'icons' array in function below
#define YPOS   1
#define DELTAY 2

void testanimate(const uint8_t *bitmap, uint8_t w, uint8_t h) {
  int8_t f, icons[NUMFLAKES][3];

  // Initialize 'snowflake' positions
  for(f=0; f< NUMFLAKES; f++) {
    icons[f][XPOS]   = random(1 - LOGO_WIDTH, display.width());
    icons[f][YPOS]   = -LOGO_HEIGHT;
    icons[f][DELTAY] = random(1, 6);
    Serial.print(F("x: "));
    Serial.print(icons[f][XPOS], DEC);
    Serial.print(F(" y: "));
    Serial.print(icons[f][YPOS], DEC);
    Serial.print(F(" dy: "));
    Serial.println(icons[f][DELTAY], DEC);
  }

  for(;;) { // Loop forever...
    display.clearDisplay(); // Clear the display buffer

    // Draw each snowflake:
    for(f=0; f< NUMFLAKES; f++) {
      display.drawBitmap(icons[f][XPOS], icons[f][YPOS], bitmap, w, h, WHITE);
    }

    display.display(); // Show the display buffer on the screen
    delay(200);        // Pause for 1/10 second

    // Then update coordinates of each flake...
    for(f=0; f< NUMFLAKES; f++) {
      icons[f][YPOS] += icons[f][DELTAY];
      // If snowflake is off the bottom of the screen...
      if (icons[f][YPOS] >= display.height()) {
        // Reinitialize to a random position, just off the top
        icons[f][XPOS]   = random(1 - LOGO_WIDTH, display.width());
        icons[f][YPOS]   = -LOGO_HEIGHT;
        icons[f][DELTAY] = random(1, 6);
      }
    }
  }
}