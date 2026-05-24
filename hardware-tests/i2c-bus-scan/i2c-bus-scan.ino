#include <Wire.h>

const unsigned long SERIAL_WAIT_TIMEOUT_MS = 3000;
const unsigned long SCAN_INTERVAL_MS = 5000;

unsigned long lastScanMs = 0;
bool scanHasRun = false;

void printHexAddress(uint8_t address) {
  Serial.print(F("0x"));
  if (address < 0x10) {
    Serial.print(F("0"));
  }
  Serial.print(address, HEX);
}

void printLikelyDeviceHint(uint8_t address) {
  switch (address) {
    case 0x29:
      Serial.print(F(" - likely TCS34725 color sensor"));
      break;
    case 0x44:
    case 0x45:
      Serial.print(F(" - likely SHT4x/SHT45 depending on module/address configuration"));
      break;
    case 0x57:
      Serial.print(F(" - likely AT24C32 EEPROM"));
      break;
    case 0x68:
      Serial.print(F(" - likely DS3231 RTC"));
      break;
    default:
      Serial.print(F(" - no typical AutoTerra device hint"));
      break;
  }
}

void printI2cErrorHint(uint8_t error) {
  switch (error) {
    case 0:
      Serial.print(F("success"));
      break;
    case 1:
      Serial.print(F("data too long"));
      break;
    case 2:
      Serial.print(F("NACK on address"));
      break;
    case 3:
      Serial.print(F("NACK on data"));
      break;
    case 4:
      Serial.print(F("other error"));
      break;
    default:
      Serial.print(F("unknown status"));
      break;
  }
}

void scanI2cBus() {
  uint8_t foundCount = 0;

  Serial.println();
  Serial.println(F("--- I2C scan ---"));

  for (uint8_t address = 0x03; address <= 0x77; address++) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
      foundCount++;
      Serial.print(F("Found device at "));
      printHexAddress(address);
      printLikelyDeviceHint(address);
      Serial.println();
    } else if (error == 4) {
      Serial.print(F("Unknown I2C error at "));
      printHexAddress(address);
      Serial.print(F(": "));
      printI2cErrorHint(error);
      Serial.println();
    }
  }

  Serial.print(F("Devices found: "));
  Serial.println(foundCount);

  if (foundCount == 0) {
    Serial.println(F("WARNING: No I2C devices found. Check wiring, power, common ground, and 3.3 V pull-ups."));
  }

  Serial.println(F("Scan complete. Next scan in 5 seconds."));
}

void setup() {
  Serial.begin(115200);

  unsigned long serialStartMs = millis();
  while (!Serial && (millis() - serialStartMs < SERIAL_WAIT_TIMEOUT_MS)) {
    // Wait briefly for the Serial Monitor without blocking standalone operation forever.
  }

  Serial.println();
  Serial.println(F("AutoTerra manual hardware test: I2C bus scan"));
  Serial.println(F("Target board: Arduino Nano 33 IoT"));
  Serial.println(F("I2C pins: A4 = SDA, A5 = SCL"));
  Serial.println(F("WARNING: Do not use 5 V pull-ups on SDA/SCL. Use effective pull-ups to 3.3 V."));

  Wire.begin();
}

void loop() {
  unsigned long nowMs = millis();

  if (!scanHasRun || (nowMs - lastScanMs >= SCAN_INTERVAL_MS)) {
    lastScanMs = nowMs;
    scanHasRun = true;
    scanI2cBus();
  }
}
