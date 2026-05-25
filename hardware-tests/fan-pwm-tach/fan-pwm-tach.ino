#include <Arduino.h>

const uint8_t PIN_FAN_PWM = 6;
const uint8_t PIN_FAN1_TACH = 9;
const uint8_t PIN_FAN2_TACH = 10;

const unsigned long SERIAL_WAIT_TIMEOUT_MS = 3000;
const unsigned long STEP_DURATION_MS = 10000;
const unsigned long REPORT_INTERVAL_MS = 2000;
const uint8_t PULSES_PER_REVOLUTION = 2;

const uint8_t TEST_EFFECTIVE_PERCENTS[] = {0, 15, 25, 50, 75, 100};
const uint8_t TEST_STEP_COUNT = sizeof(TEST_EFFECTIVE_PERCENTS) / sizeof(TEST_EFFECTIVE_PERCENTS[0]);

volatile unsigned long fan1PulseCount = 0;
volatile unsigned long fan2PulseCount = 0;

uint8_t currentStepIndex = 0;
uint8_t currentEffectivePercent = 0;
uint8_t currentArduinoPwm = 255;

unsigned long currentStepStartedMs = 0;
unsigned long lastReportMs = 0;
unsigned long lastFan1PulseCount = 0;
unsigned long lastFan2PulseCount = 0;
bool tachInterruptsReady = false;

void fan1TachIsr() {
  fan1PulseCount++;
}

void fan2TachIsr() {
  fan2PulseCount++;
}

uint8_t effectivePercentToArduinoPwm(uint8_t effectivePercent) {
  if (effectivePercent > 100) {
    effectivePercent = 100;
  }

  // The 2N3904 collector stage inverts the signal before it reaches the fan PWM wire.
  return 255 - ((static_cast<uint16_t>(effectivePercent) * 255) / 100);
}

void applyEffectiveFanPercent(uint8_t effectivePercent) {
  currentEffectivePercent = effectivePercent;
  currentArduinoPwm = effectivePercentToArduinoPwm(effectivePercent);
  analogWrite(PIN_FAN_PWM, currentArduinoPwm);
}

void printStartupBanner() {
  Serial.println();
  Serial.println(F("AutoTerra manual hardware test: fan PWM and tach"));
  Serial.println(F("Target board: Arduino Nano 33 IoT"));
  Serial.println(F("PWM pin: D6 shared fan PWM driver through inverting 2N3904 stage"));
  Serial.println(F("Tach pins: D9 fan 1 tach, D10 fan 2 tach"));
  Serial.println(F("This first test uses Arduino analogWrite() and does not validate the final 25 kHz PWM requirement."));
  Serial.println(F("WARNING: D6 uses an inverting transistor driver."));
  Serial.println(F("WARNING: The fan PWM collector node is pulled up to 5 V and must not connect directly to a Nano GPIO."));
  Serial.println(F("WARNING: Tach pull-ups must be external 10 kOhm pull-ups to 3.3 V, not 5 V."));
  Serial.println(F("Tach outputs must remain separate. Do not tie fan 1 and fan 2 tach together."));
  Serial.println(F("RPM calculation assumes two tach pulses per revolution unless hardware testing proves otherwise."));
  Serial.print(F("Startup effective fan percent: "));
  Serial.print(currentEffectivePercent);
  Serial.print(F("%, Arduino PWM written to D6: "));
  Serial.println(currentArduinoPwm);
}

bool configureTachInterrupts() {
  int fan1Interrupt = digitalPinToInterrupt(PIN_FAN1_TACH);
  int fan2Interrupt = digitalPinToInterrupt(PIN_FAN2_TACH);

  if (fan1Interrupt == NOT_AN_INTERRUPT || fan2Interrupt == NOT_AN_INTERRUPT) {
    Serial.println(F("ERROR: One or both tach pins do not map to external interrupts on this board."));
    Serial.print(F("D9 interrupt number: "));
    Serial.println(fan1Interrupt);
    Serial.print(F("D10 interrupt number: "));
    Serial.println(fan2Interrupt);
    Serial.println(F("RPM will not be reported. Move no wiring unless the hardware documents are updated deliberately."));
    return false;
  }

  attachInterrupt(fan1Interrupt, fan1TachIsr, FALLING);
  attachInterrupt(fan2Interrupt, fan2TachIsr, FALLING);
  Serial.println(F("Tach interrupts attached on falling edges."));
  return true;
}

unsigned long readFan1Pulses() {
  noInterrupts();
  unsigned long pulses = fan1PulseCount;
  interrupts();
  return pulses;
}

unsigned long readFan2Pulses() {
  noInterrupts();
  unsigned long pulses = fan2PulseCount;
  interrupts();
  return pulses;
}

float calculateRpm(unsigned long pulses, unsigned long intervalMs) {
  if (intervalMs == 0) {
    return 0.0f;
  }

  return (static_cast<float>(pulses) * 60000.0f) /
         (static_cast<float>(PULSES_PER_REVOLUTION) * static_cast<float>(intervalMs));
}

void printReport(unsigned long nowMs) {
  unsigned long intervalMs = nowMs - lastReportMs;

  Serial.print(F("effective="));
  Serial.print(currentEffectivePercent);
  Serial.print(F("%, arduino_pwm="));
  Serial.print(currentArduinoPwm);

  if (tachInterruptsReady) {
    unsigned long fan1Pulses = readFan1Pulses();
    unsigned long fan2Pulses = readFan2Pulses();
    unsigned long fan1Delta = fan1Pulses - lastFan1PulseCount;
    unsigned long fan2Delta = fan2Pulses - lastFan2PulseCount;

    lastFan1PulseCount = fan1Pulses;
    lastFan2PulseCount = fan2Pulses;

    Serial.print(F(", fan1_pulses="));
    Serial.print(fan1Delta);
    Serial.print(F(", fan1_rpm="));
    Serial.print(calculateRpm(fan1Delta, intervalMs), 1);
    Serial.print(F(", fan2_pulses="));
    Serial.print(fan2Delta);
    Serial.print(F(", fan2_rpm="));
    Serial.print(calculateRpm(fan2Delta, intervalMs), 1);
  } else {
    Serial.print(F(", fan1_pulses=unsupported, fan1_rpm=unsupported"));
    Serial.print(F(", fan2_pulses=unsupported, fan2_rpm=unsupported"));
  }

  Serial.println();
}

void advanceStep(unsigned long nowMs) {
  currentStepIndex = (currentStepIndex + 1) % TEST_STEP_COUNT;
  applyEffectiveFanPercent(TEST_EFFECTIVE_PERCENTS[currentStepIndex]);
  currentStepStartedMs = nowMs;

  Serial.println();
  Serial.print(F("--- New fan step: effective="));
  Serial.print(currentEffectivePercent);
  Serial.print(F("%, Arduino PWM written to D6="));
  Serial.print(currentArduinoPwm);
  Serial.println(F(" ---"));
}

void setup() {
  digitalWrite(PIN_FAN_PWM, HIGH);
  pinMode(PIN_FAN_PWM, OUTPUT);
  pinMode(PIN_FAN1_TACH, INPUT);
  pinMode(PIN_FAN2_TACH, INPUT);

  applyEffectiveFanPercent(0);

  Serial.begin(115200);
  unsigned long serialStartMs = millis();
  while (!Serial && (millis() - serialStartMs < SERIAL_WAIT_TIMEOUT_MS)) {
    // Wait briefly for the Serial Monitor without blocking standalone operation forever.
  }

  printStartupBanner();
  tachInterruptsReady = configureTachInterrupts();

  unsigned long nowMs = millis();
  currentStepStartedMs = nowMs;
  lastReportMs = nowMs;

  Serial.println(F("Test sequence repeats continuously: 0%, 15%, 25%, 50%, 75%, 100%."));
  Serial.println(F("Each step is held for 10 seconds. Tach reports print every 2 seconds."));
}

void loop() {
  unsigned long nowMs = millis();

  if (nowMs - currentStepStartedMs >= STEP_DURATION_MS) {
    advanceStep(nowMs);
  }

  if (nowMs - lastReportMs >= REPORT_INTERVAL_MS) {
    printReport(nowMs);
    lastReportMs = nowMs;
  }
}
