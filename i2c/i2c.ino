#include <Wire.h>

// Set I2C bus to use: Wire, Wire1, etc.
#define WIRE Wire

void setup() {
  WIRE.begin();
  // NOTE: Do NOT enable internal pull-ups (they pull to 5V).
  // Use external 4.7k resistors to 3.3V instead.

  Serial.begin(115200);
  while (!Serial)
     delay(10);
  Serial.println("\nI2C Scanner");
}


void loop() {
  byte error, address;
  int nDevices;

  // Read and print voltage on SDA and SCL lines
  float sdaVoltage = analogRead(A4) * (5.0 / 1023.0);
  float sclVoltage = analogRead(A5) * (5.0 / 1023.0);
  Serial.print("SDA (A4) voltage: ");
  Serial.print(sdaVoltage, 2);
  Serial.print("V  |  SCL (A5) voltage: ");
  Serial.print(sclVoltage, 2);
  Serial.println("V");

  // Re-enable I2C after analogRead took over the pins
  WIRE.begin();

  Serial.print("Scanning...");

  nDevices = 0;
  for(address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    WIRE.beginTransmission(address);
    error = WIRE.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error==4)
    {
      Serial.print("Unknown error at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");

  delay(1000);           // wait 5 seconds for next scan
}