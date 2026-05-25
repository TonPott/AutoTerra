#include <Arduino.h>

#if defined(ARDUINO_ARCH_SAMD)
#include "wiring_private.h"
#endif

const uint8_t PIN_FAN_PWM = 6;
const uint8_t PIN_FAN1_TACH = 9;
const uint8_t PIN_FAN2_TACH = 10;

const unsigned long SERIAL_WAIT_TIMEOUT_MS = 3000;
const unsigned long STEP_DURATION_MS = 12000;
const unsigned long REPORT_INTERVAL_MS = 2000;
const unsigned long SUMMARY_WINDOW_MS = 6000;

const uint8_t PULSES_PER_REVOLUTION = 2;
const uint8_t TEST_EFFECTIVE_PERCENTS[] = {0, 8, 10, 15, 20, 30, 50, 75, 100};
const uint8_t TEST_STEP_COUNT = sizeof(TEST_EFFECTIVE_PERCENTS) / sizeof(TEST_EFFECTIVE_PERCENTS[0]);

#define FAN_PWM_MODE_ANALOGWRITE 1
#define FAN_PWM_MODE_25KHZ 2

// Upload once with FAN_PWM_MODE_ANALOGWRITE and once with FAN_PWM_MODE_25KHZ,
// then compare the Serial output and manual acoustic notes.
#ifndef FAN_PWM_MODE
#define FAN_PWM_MODE FAN_PWM_MODE_ANALOGWRITE
#endif

#if defined(ARDUINO_ARCH_SAMD)
const uint32_t TCC0_25KHZ_PER = 1919;  // 48 MHz / (1919 + 1) = 25,000 Hz.
#endif

volatile unsigned long fan1PulseCount = 0;
volatile unsigned long fan2PulseCount = 0;

struct RpmStats {
  uint16_t sampleCount;
  float fan1Sum;
  float fan2Sum;
  float fan1Min;
  float fan1Max;
  float fan2Min;
  float fan2Max;
};

uint8_t currentStepIndex = 0;
uint8_t currentEffectivePercent = 0;
uint8_t currentArduinoPwm = 255;
uint32_t currentPwmCompareValue = 255;
unsigned long stepStartedMs = 0;
unsigned long lastReportMs = 0;
unsigned long lastFan1PulseCount = 0;
unsigned long lastFan2PulseCount = 0;
bool fatalError = false;
bool testComplete = false;
RpmStats currentStats;

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

  // The 2N3904 collector stage inverts the PWM signal before it reaches the fans.
  return 255 - ((static_cast<uint16_t>(effectivePercent) * 255) / 100);
}

const __FlashStringHelper* selectedModeName() {
#if FAN_PWM_MODE == FAN_PWM_MODE_25KHZ
  return F("25KHZ");
#else
  return F("ANALOGWRITE");
#endif
}

void resetStats() {
  currentStats.sampleCount = 0;
  currentStats.fan1Sum = 0.0f;
  currentStats.fan2Sum = 0.0f;
  currentStats.fan1Min = 0.0f;
  currentStats.fan1Max = 0.0f;
  currentStats.fan2Min = 0.0f;
  currentStats.fan2Max = 0.0f;
}

void addStatsSample(float fan1Rpm, float fan2Rpm) {
  if (currentStats.sampleCount == 0) {
    currentStats.fan1Min = fan1Rpm;
    currentStats.fan1Max = fan1Rpm;
    currentStats.fan2Min = fan2Rpm;
    currentStats.fan2Max = fan2Rpm;
  } else {
    if (fan1Rpm < currentStats.fan1Min) {
      currentStats.fan1Min = fan1Rpm;
    }
    if (fan1Rpm > currentStats.fan1Max) {
      currentStats.fan1Max = fan1Rpm;
    }
    if (fan2Rpm < currentStats.fan2Min) {
      currentStats.fan2Min = fan2Rpm;
    }
    if (fan2Rpm > currentStats.fan2Max) {
      currentStats.fan2Max = fan2Rpm;
    }
  }

  currentStats.sampleCount++;
  currentStats.fan1Sum += fan1Rpm;
  currentStats.fan2Sum += fan2Rpm;
}

float averageFan1Rpm() {
  if (currentStats.sampleCount == 0) {
    return 0.0f;
  }
  return currentStats.fan1Sum / static_cast<float>(currentStats.sampleCount);
}

float averageFan2Rpm() {
  if (currentStats.sampleCount == 0) {
    return 0.0f;
  }
  return currentStats.fan2Sum / static_cast<float>(currentStats.sampleCount);
}

void printFloatCsv(float value) {
  Serial.print(value, 1);
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

#if defined(ARDUINO_ARCH_SAMD)
void syncTcc0() {
  while (TCC0->SYNCBUSY.reg & TCC_SYNCBUSY_MASK) {
  }
}

bool d6MapsToExpectedTcc0Channel() {
  const PinDescription& pinDescription = g_APinDescription[PIN_FAN_PWM];
  return pinDescription.ulPort == PORTA &&
         pinDescription.ulPin == 4 &&
         pinDescription.ulPWMChannel == PWM0_CH0 &&
         pinDescription.ulTCChannel == TCC0_CH0;
}

uint32_t arduinoPwmToTccCompare(uint8_t arduinoPwmValue) {
  uint32_t compare = ((static_cast<uint32_t>(arduinoPwmValue) * TCC0_25KHZ_PER) + 127) / 255;
  if (compare > TCC0_25KHZ_PER) {
    compare = TCC0_25KHZ_PER;
  }
  return compare;
}

bool configureTcc0For25KhzD6() {
  if (!d6MapsToExpectedTcc0Channel()) {
    Serial.println(F("FATAL: D6 does not map to PA04 TCC0/WO0 as expected for Arduino Nano 33 IoT."));
    Serial.println(F("25 kHz mode will not be faked."));
    return false;
  }

  PM->APBCMASK.reg |= PM_APBCMASK_TCC0;
  GCLK->CLKCTRL.reg = static_cast<uint16_t>(GCLK_CLKCTRL_CLKEN |
                                            GCLK_CLKCTRL_GEN_GCLK0 |
                                            GCLK_CLKCTRL_ID(GCM_TCC0_TCC1));
  while (GCLK->STATUS.bit.SYNCBUSY == 1) {
  }

  TCC0->CTRLA.bit.ENABLE = 0;
  syncTcc0();
  TCC0->CTRLA.bit.SWRST = 1;
  syncTcc0();
  while (TCC0->CTRLA.bit.SWRST) {
  }

  TCC0->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV1;
  TCC0->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
  syncTcc0();
  TCC0->PER.reg = TCC0_25KHZ_PER;
  syncTcc0();
  TCC0->CC[0].reg = arduinoPwmToTccCompare(currentArduinoPwm);
  syncTcc0();

  pinPeripheral(PIN_FAN_PWM, PIO_TIMER);

  TCC0->CTRLA.bit.ENABLE = 1;
  syncTcc0();
  return true;
}

void writeTcc0Duty(uint8_t arduinoPwmValue) {
  currentPwmCompareValue = arduinoPwmToTccCompare(arduinoPwmValue);
  TCC0->CTRLBSET.bit.LUPD = 1;
  syncTcc0();
  TCC0->CCB[0].reg = currentPwmCompareValue;
  syncTcc0();
  TCC0->CTRLBCLR.bit.LUPD = 1;
  syncTcc0();
}
#endif

bool configureSelectedPwmMode() {
#if FAN_PWM_MODE == FAN_PWM_MODE_25KHZ
#if defined(ARDUINO_ARCH_SAMD)
  currentPwmCompareValue = arduinoPwmToTccCompare(currentArduinoPwm);
  return configureTcc0For25KhzD6();
#else
  Serial.println(F("FATAL: 25 kHz mode is only implemented for SAMD boards in this test sketch."));
  Serial.println(F("25 kHz mode will not be faked."));
  return false;
#endif
#else
  pinMode(PIN_FAN_PWM, OUTPUT);
  analogWrite(PIN_FAN_PWM, currentArduinoPwm);
  currentPwmCompareValue = currentArduinoPwm;
  return true;
#endif
}

void applyEffectiveFanPercent(uint8_t effectivePercent) {
  currentEffectivePercent = effectivePercent;
  currentArduinoPwm = effectivePercentToArduinoPwm(effectivePercent);

#if FAN_PWM_MODE == FAN_PWM_MODE_25KHZ
#if defined(ARDUINO_ARCH_SAMD)
  writeTcc0Duty(currentArduinoPwm);
#endif
#else
  analogWrite(PIN_FAN_PWM, currentArduinoPwm);
  currentPwmCompareValue = currentArduinoPwm;
#endif
}

bool configureTachInterrupts() {
  int fan1Interrupt = digitalPinToInterrupt(PIN_FAN1_TACH);
  int fan2Interrupt = digitalPinToInterrupt(PIN_FAN2_TACH);

  if (fan1Interrupt == NOT_AN_INTERRUPT || fan2Interrupt == NOT_AN_INTERRUPT) {
    Serial.println(F("FATAL: One or both tach pins do not map to external interrupts on this board."));
    Serial.print(F("D9 interrupt number: "));
    Serial.println(fan1Interrupt);
    Serial.print(F("D10 interrupt number: "));
    Serial.println(fan2Interrupt);
    Serial.println(F("RPM will not be faked. Stopping the test at effective 0%."));
    return false;
  }

  attachInterrupt(fan1Interrupt, fan1TachIsr, FALLING);
  attachInterrupt(fan2Interrupt, fan2TachIsr, FALLING);
  Serial.println(F("Tach interrupts attached on falling edges."));
  return true;
}

void printStartupBanner() {
  Serial.println();
  Serial.println(F("AutoTerra manual hardware test: fan PWM frequency A/B"));
  Serial.println(F("Target board: Arduino Nano 33 IoT"));
  Serial.println(F("PWM pin: D6 shared fan PWM driver through inverting 2N3904 stage"));
  Serial.println(F("Tach pins: D9 fan 1 tach, D10 fan 2 tach"));
  Serial.print(F("Selected PWM mode: "));
  Serial.println(selectedModeName());

#if FAN_PWM_MODE == FAN_PWM_MODE_25KHZ
#if defined(ARDUINO_ARCH_SAMD)
  Serial.println(F("25 kHz mode: local SAMD21 TCC0 code for D6 PA04 TCC0/WO0."));
  Serial.println(F("Target PWM frequency: 25000 Hz using GCLK0 48 MHz and TCC0 PER=1919."));
#else
  Serial.println(F("25 kHz mode: unavailable on this non-SAMD build."));
#endif
#else
  Serial.println(F("25 kHz mode: not active in this upload."));
#endif

  Serial.println(F("External timer/PWM library: none."));
  Serial.println(F("Mode switching: compile-time selection. Upload once per mode and compare outputs manually."));
  Serial.println(F("WARNING: D6 uses an inverting transistor driver."));
  Serial.println(F("WARNING: The fan PWM collector node is pulled up to 5 V and must not connect directly to a Nano GPIO."));
  Serial.println(F("WARNING: D9 and D10 require external 10 kOhm pull-ups to 3.3 V. Internal pull-ups are not enabled."));
  Serial.println(F("WARNING: Tach outputs must remain separate and must not be tied together."));
  Serial.println(F("Manual acoustic perception must be recorded outside firmware."));
  Serial.println();
  Serial.println(F("STATUS,mode,percent,pwm_or_compare_value,elapsed_ms,fan1_pulses,fan1_rpm,fan2_pulses,fan2_rpm,used_for_summary"));
  Serial.println(F("AB_RESULT,mode,percent,pwm_or_compare_value,fan1_avg_rpm,fan2_avg_rpm,fan1_min_rpm,fan1_max_rpm,fan2_min_rpm,fan2_max_rpm,manual_noise_note"));
}

bool currentSampleIsUsedForSummary(unsigned long elapsedMs) {
  return elapsedMs >= (STEP_DURATION_MS - SUMMARY_WINDOW_MS);
}

void printStatusReport(unsigned long nowMs) {
  unsigned long intervalMs = nowMs - lastReportMs;
  unsigned long elapsedMs = nowMs - stepStartedMs;

  unsigned long fan1Pulses = readFan1Pulses();
  unsigned long fan2Pulses = readFan2Pulses();
  unsigned long fan1Delta = fan1Pulses - lastFan1PulseCount;
  unsigned long fan2Delta = fan2Pulses - lastFan2PulseCount;
  float fan1Rpm = calculateRpm(fan1Delta, intervalMs);
  float fan2Rpm = calculateRpm(fan2Delta, intervalMs);
  bool usedForSummary = currentSampleIsUsedForSummary(elapsedMs);

  if (usedForSummary) {
    addStatsSample(fan1Rpm, fan2Rpm);
  }

  Serial.print(F("STATUS,"));
  Serial.print(selectedModeName());
  Serial.print(F(","));
  Serial.print(currentEffectivePercent);
  Serial.print(F(","));
  Serial.print(currentPwmCompareValue);
  Serial.print(F(","));
  Serial.print(elapsedMs);
  Serial.print(F(","));
  Serial.print(fan1Delta);
  Serial.print(F(","));
  printFloatCsv(fan1Rpm);
  Serial.print(F(","));
  Serial.print(fan2Delta);
  Serial.print(F(","));
  printFloatCsv(fan2Rpm);
  Serial.print(F(","));
  Serial.print(usedForSummary ? F("1") : F("0"));
  Serial.println();

  lastReportMs = nowMs;
  lastFan1PulseCount = fan1Pulses;
  lastFan2PulseCount = fan2Pulses;
}

void printStepSummary() {
  Serial.print(F("AB_RESULT,"));
  Serial.print(selectedModeName());
  Serial.print(F(","));
  Serial.print(currentEffectivePercent);
  Serial.print(F(","));
  Serial.print(currentPwmCompareValue);
  Serial.print(F(","));
  printFloatCsv(averageFan1Rpm());
  Serial.print(F(","));
  printFloatCsv(averageFan2Rpm());
  Serial.print(F(","));
  printFloatCsv(currentStats.fan1Min);
  Serial.print(F(","));
  printFloatCsv(currentStats.fan1Max);
  Serial.print(F(","));
  printFloatCsv(currentStats.fan2Min);
  Serial.print(F(","));
  printFloatCsv(currentStats.fan2Max);
  Serial.print(F(","));
  Serial.println();
}

void beginStep(uint8_t stepIndex, unsigned long nowMs) {
  currentStepIndex = stepIndex;
  applyEffectiveFanPercent(TEST_EFFECTIVE_PERCENTS[currentStepIndex]);
  resetStats();
  stepStartedMs = nowMs;
  lastReportMs = nowMs;
  lastFan1PulseCount = readFan1Pulses();
  lastFan2PulseCount = readFan2Pulses();

  Serial.println();
  Serial.print(F("STEP_BEGIN,"));
  Serial.print(selectedModeName());
  Serial.print(F(",percent="));
  Serial.print(currentEffectivePercent);
  Serial.print(F(",pwm_or_compare_value="));
  Serial.println(currentPwmCompareValue);
}

void setup() {
  digitalWrite(PIN_FAN_PWM, HIGH);
  pinMode(PIN_FAN_PWM, OUTPUT);
  pinMode(PIN_FAN1_TACH, INPUT);
  pinMode(PIN_FAN2_TACH, INPUT);

  currentEffectivePercent = 0;
  currentArduinoPwm = effectivePercentToArduinoPwm(0);
  currentPwmCompareValue = currentArduinoPwm;

  Serial.begin(115200);
  unsigned long serialStartMs = millis();
  while (!Serial && (millis() - serialStartMs < SERIAL_WAIT_TIMEOUT_MS)) {
    // Wait briefly for the Serial Monitor without blocking standalone operation forever.
  }

  printStartupBanner();

  if (!configureSelectedPwmMode()) {
    fatalError = true;
    digitalWrite(PIN_FAN_PWM, HIGH);
    return;
  }

  applyEffectiveFanPercent(0);

  if (!configureTachInterrupts()) {
    fatalError = true;
    applyEffectiveFanPercent(0);
    return;
  }

  beginStep(0, millis());
}

void loop() {
  if (fatalError || testComplete) {
    return;
  }

  unsigned long nowMs = millis();

  if (nowMs - lastReportMs >= REPORT_INTERVAL_MS) {
    printStatusReport(nowMs);
  }

  if (nowMs - stepStartedMs >= STEP_DURATION_MS) {
    printStepSummary();
    uint8_t nextStepIndex = currentStepIndex + 1;
    if (nextStepIndex < TEST_STEP_COUNT) {
      beginStep(nextStepIndex, nowMs);
    } else {
      applyEffectiveFanPercent(0);
      Serial.println();
      Serial.println(F("TEST_COMPLETE,fan-pwm-frequency-ab"));
      Serial.println(F("Upload the other compile-time mode and compare RPM plus manual acoustic notes."));
      testComplete = true;
    }
  }
}
