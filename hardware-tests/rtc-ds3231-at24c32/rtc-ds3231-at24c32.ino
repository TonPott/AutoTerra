#include <Wire.h>
#include <RTClib.h>
#include <JC_EEPROM.h>

constexpr bool SET_RTC_FROM_COMPILE_TIME = false;

constexpr uint8_t DS3231_ADDRESS = 0x68;
constexpr uint8_t AT24C32_ADDRESS = 0x57;

constexpr uint8_t DS3231_INT_PIN = 2;
constexpr uint8_t DS3231_32KHZ_PIN = 8;

constexpr unsigned long SERIAL_WAIT_TIMEOUT_MS = 3000;
constexpr unsigned long STATUS_INTERVAL_MS = 5000;
constexpr unsigned long ALARM_OFFSET_SECONDS = 60;
constexpr unsigned long ALARM_TIMEOUT_MS = 90000;

constexpr uint32_t AT24C32_SIZE_BYTES = 4096;
constexpr uint32_t EEPROM_TEST_OFFSET = 0x0F00;
constexpr uint32_t EEPROM_TEST_MAGIC = 0xA7C3D251UL;

struct EepromTestRecord {
  uint32_t magic;
  uint16_t counter;
  char label[16];
  uint8_t pattern[4];
};

RTC_DS3231 rtc;
JC_EEPROM at24c32(JC_EEPROM::kbits_32, 1, 32, AT24C32_ADDRESS);

bool rtcAddressOk = false;
bool rtcBeginOk = false;
bool rtcLostPower = false;
bool rtc32khzEnabled = false;

bool eepromAddressOk = false;
bool eepromBeginOk = false;
bool eepromVerifyOk = false;

bool alarmSetupOk = false;
bool alarmActive = false;
bool alarmFired = false;
bool alarmCleared = false;
DateTime alarmTarget;
unsigned long alarmStartMs = 0;

unsigned long lastStatusMs = 0;

void printHexByte(uint8_t value) {
  Serial.print(F("0x"));
  if (value < 0x10) {
    Serial.print(F("0"));
  }
  Serial.print(value, HEX);
}

void printHexWord(uint32_t value) {
  Serial.print(F("0x"));
  if (value < 0x1000) {
    Serial.print(F("0"));
  }
  if (value < 0x0100) {
    Serial.print(F("0"));
  }
  if (value < 0x0010) {
    Serial.print(F("0"));
  }
  Serial.print(value, HEX);
}

void printDateTime(const DateTime& value) {
  Serial.print(value.year());
  Serial.print(F("-"));
  if (value.month() < 10) {
    Serial.print(F("0"));
  }
  Serial.print(value.month());
  Serial.print(F("-"));
  if (value.day() < 10) {
    Serial.print(F("0"));
  }
  Serial.print(value.day());
  Serial.print(F(" "));
  if (value.hour() < 10) {
    Serial.print(F("0"));
  }
  Serial.print(value.hour());
  Serial.print(F(":"));
  if (value.minute() < 10) {
    Serial.print(F("0"));
  }
  Serial.print(value.minute());
  Serial.print(F(":"));
  if (value.second() < 10) {
    Serial.print(F("0"));
  }
  Serial.print(value.second());
}

void printPinState(uint8_t pin) {
  Serial.print(digitalRead(pin) == LOW ? F("LOW") : F("HIGH"));
}

bool i2cAddressResponds(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

void printAddressStatus(const __FlashStringHelper* label, uint8_t address, bool responding) {
  Serial.print(label);
  Serial.print(F(" address "));
  printHexByte(address);
  Serial.print(F(": "));
  Serial.println(responding ? F("responding") : F("not responding"));
}

bool recordsMatch(const EepromTestRecord& expected, const EepromTestRecord& actual) {
  if (expected.magic != actual.magic || expected.counter != actual.counter) {
    return false;
  }

  for (size_t index = 0; index < sizeof(expected.label); index++) {
    if (expected.label[index] != actual.label[index]) {
      return false;
    }
  }

  for (size_t index = 0; index < sizeof(expected.pattern); index++) {
    if (expected.pattern[index] != actual.pattern[index]) {
      return false;
    }
  }

  return true;
}

void printStartupBanner() {
  Serial.println();
  Serial.println(F("AutoTerra manual hardware test: DS3231 RTC + AT24C32 EEPROM"));
  Serial.println(F("Target board: Arduino Nano 33 IoT"));
  Serial.println(F("I2C pins: A4 = SDA, A5 = SCL"));
  Serial.println(F("Tested pins: D2 = DS3231 INT/SQW, D8 = prepared DS3231 32kHz input"));
  Serial.println(F("Serial baud: 115200"));
  Serial.println(F("RTC time is read-only by default."));
  Serial.print(F("EEPROM write warning: one test record will be written at "));
  printHexWord(EEPROM_TEST_OFFSET);
  Serial.println(F("."));

  if (SET_RTC_FROM_COMPILE_TIME) {
    Serial.println(F("WARNING: SET_RTC_FROM_COMPILE_TIME is true. RTC time will be overwritten from build time."));
  }
}

void initializeRtc() {
  rtcAddressOk = i2cAddressResponds(DS3231_ADDRESS);
  printAddressStatus(F("DS3231 RTC"), DS3231_ADDRESS, rtcAddressOk);

  rtcBeginOk = rtc.begin();
  Serial.print(F("RTClib DS3231 begin: "));
  Serial.println(rtcBeginOk ? F("ok") : F("failed"));

  if (!rtcBeginOk) {
    Serial.println(F("RTC time, lost-power status, 32kHz status, and alarm setup are unavailable."));
    return;
  }

  rtcLostPower = rtc.lostPower();
  Serial.print(F("RTC lost power flag: "));
  Serial.println(rtcLostPower ? F("true") : F("false"));

  if (SET_RTC_FROM_COMPILE_TIME) {
    Serial.println(F("WARNING: Setting RTC time from __DATE__ and __TIME__ now."));
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  rtc32khzEnabled = rtc.isEnabled32K();
  Serial.print(F("DS3231 32kHz output enabled flag: "));
  Serial.println(rtc32khzEnabled ? F("true") : F("false"));

  DateTime now = rtc.now();
  Serial.print(F("Current RTC time: "));
  printDateTime(now);
  Serial.println();
}

void setupAlarmTest() {
  if (!rtcBeginOk) {
    Serial.println(F("Alarm test skipped because RTC initialization failed."));
    return;
  }

  // INT/SQW is open-drain and active LOW when an enabled alarm flag is set.
  rtc.disableAlarm(1);
  rtc.disableAlarm(2);
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);
  rtc.writeSqwPinMode(DS3231_OFF);

  DateTime now = rtc.now();
  alarmTarget = now + TimeSpan(ALARM_OFFSET_SECONDS);
  alarmSetupOk = rtc.setAlarm1(alarmTarget, DS3231_A1_Date);

  Serial.print(F("Alarm1 target: "));
  printDateTime(alarmTarget);
  Serial.println();
  Serial.print(F("Alarm1 setup through RTClib: "));
  Serial.println(alarmSetupOk ? F("ok") : F("failed"));

  if (alarmSetupOk) {
    alarmActive = true;
    alarmStartMs = millis();
    Serial.println(F("Alarm test is polling D2 and the DS3231 alarm flag. No interrupt handler is attached."));
  }
}

void runEepromTest() {
  eepromAddressOk = i2cAddressResponds(AT24C32_ADDRESS);
  printAddressStatus(F("AT24C32 EEPROM"), AT24C32_ADDRESS, eepromAddressOk);

  if (!eepromAddressOk) {
    Serial.println(F("EEPROM test skipped because the expected address did not respond."));
    return;
  }

  uint8_t beginStatus = at24c32.begin(JC_EEPROM::twiClock100kHz);
  eepromBeginOk = beginStatus == 0;
  Serial.print(F("JC_EEPROM begin status: "));
  Serial.println(beginStatus);

  if (!eepromBeginOk) {
    Serial.println(F("EEPROM test skipped because JC_EEPROM initialization failed."));
    return;
  }

  if (EEPROM_TEST_OFFSET + sizeof(EepromTestRecord) > AT24C32_SIZE_BYTES) {
    Serial.println(F("EEPROM test skipped because the configured record does not fit in AT24C32."));
    return;
  }

  EepromTestRecord writeRecord = {
    EEPROM_TEST_MAGIC,
    1,
    "RTC_EEPROM_TEST",
    {0x5A, 0xA5, 0x33, 0xCC}
  };
  EepromTestRecord readRecord = {};

  Serial.print(F("Writing AT24C32 test record at offset "));
  printHexWord(EEPROM_TEST_OFFSET);
  Serial.print(F(" ("));
  Serial.print(sizeof(EepromTestRecord));
  Serial.println(F(" bytes)."));

  uint8_t writeStatus = at24c32.write(
    EEPROM_TEST_OFFSET,
    reinterpret_cast<uint8_t*>(&writeRecord),
    sizeof(writeRecord)
  );
  uint8_t readStatus = at24c32.read(
    EEPROM_TEST_OFFSET,
    reinterpret_cast<uint8_t*>(&readRecord),
    sizeof(readRecord)
  );

  eepromVerifyOk = writeStatus == 0 && readStatus == 0 && recordsMatch(writeRecord, readRecord);

  Serial.print(F("EEPROM write status: "));
  Serial.println(writeStatus);
  Serial.print(F("EEPROM read status: "));
  Serial.println(readStatus);
  Serial.print(F("EEPROM verification: "));
  Serial.println(eepromVerifyOk ? F("passed") : F("failed"));
}

void updateAlarmTest() {
  if (!alarmActive || alarmFired || !rtcBeginOk) {
    return;
  }

  bool alarmFlag = rtc.alarmFired(1);
  bool intPinActive = digitalRead(DS3231_INT_PIN) == LOW;

  if (alarmFlag || intPinActive) {
    alarmFired = true;
    alarmActive = false;

    Serial.println();
    Serial.println(F("Alarm1 fired."));
    Serial.print(F("Alarm1 flag: "));
    Serial.println(alarmFlag ? F("set") : F("not set"));
    Serial.print(F("D2 INT/SQW at alarm: "));
    printPinState(DS3231_INT_PIN);
    Serial.println();

    rtc.clearAlarm(1);
    rtc.disableAlarm(1);
    alarmCleared = true;
    Serial.println(F("Alarm1 flag cleared and Alarm1 disabled."));
    return;
  }

  if (millis() - alarmStartMs > ALARM_TIMEOUT_MS) {
    alarmActive = false;
    rtc.disableAlarm(1);
    Serial.println();
    Serial.println(F("WARNING: Alarm1 test timed out before D2 or the alarm flag indicated a trigger."));
  }
}

void printPeriodicStatus() {
  Serial.println();
  Serial.println(F("--- RTC / pin status ---"));

  if (rtcBeginOk) {
    DateTime now = rtc.now();
    Serial.print(F("RTC time: "));
    printDateTime(now);
    Serial.println();
    Serial.print(F("RTC lost power flag at startup: "));
    Serial.println(rtcLostPower ? F("true") : F("false"));
    Serial.print(F("DS3231 32kHz output enabled flag at startup: "));
    Serial.println(rtc32khzEnabled ? F("true") : F("false"));
  } else {
    Serial.println(F("RTC time: unavailable"));
  }

  Serial.print(F("D2 INT/SQW pin state: "));
  printPinState(DS3231_INT_PIN);
  Serial.println(F(" (alarm output is active LOW)"));

  Serial.print(F("D8 32kHz prepared input state: "));
  printPinState(DS3231_32KHZ_PIN);
  Serial.println(F(" (prepared optional input only; no interrupt attached)"));

  Serial.print(F("Alarm setup: "));
  Serial.println(alarmSetupOk ? F("ok") : F("not active"));
  Serial.print(F("Alarm fired: "));
  Serial.println(alarmFired ? F("yes") : F("no"));

  if (alarmActive) {
    Serial.print(F("Alarm progress: waiting for target "));
    printDateTime(alarmTarget);
    Serial.println();
  } else if (alarmCleared) {
    Serial.println(F("Alarm progress: fired and cleared."));
  }

  Serial.print(F("EEPROM address check: "));
  Serial.println(eepromAddressOk ? F("passed") : F("failed"));
  Serial.print(F("EEPROM verification: "));
  Serial.println(eepromVerifyOk ? F("passed") : F("failed or not run"));
}

void setup() {
  Serial.begin(115200);

  unsigned long serialStartMs = millis();
  while (!Serial && (millis() - serialStartMs < SERIAL_WAIT_TIMEOUT_MS)) {
    // Wait briefly for Serial Monitor without blocking standalone operation forever.
  }

  pinMode(DS3231_INT_PIN, INPUT_PULLUP);
  pinMode(DS3231_32KHZ_PIN, INPUT_PULLUP);

  printStartupBanner();

  Wire.begin();

  initializeRtc();
  setupAlarmTest();
  runEepromTest();

  lastStatusMs = millis() - STATUS_INTERVAL_MS;
}

void loop() {
  unsigned long nowMs = millis();

  updateAlarmTest();

  if (nowMs - lastStatusMs >= STATUS_INTERVAL_MS) {
    lastStatusMs = nowMs;
    printPeriodicStatus();
  }
}
