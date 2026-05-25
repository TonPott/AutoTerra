#include <Arduino.h>

const uint8_t PIN_FAN_PWM = 6;
const uint8_t PIN_FAN1_TACH = 9;
const uint8_t PIN_FAN2_TACH = 10;

const unsigned long SERIAL_WAIT_TIMEOUT_MS = 3000;
const unsigned long REPORT_INTERVAL_MS = 2000;
const unsigned long STARTUP_SPINDOWN_MS = 15000;
const unsigned long STARTUP_EVALUATION_MS = 20000;
const unsigned long RUNNING_SPINUP_MS = 10000;
const unsigned long RUNNING_EVALUATION_MS = 15000;
const unsigned long CALIBRATION_STEP_MS = 15000;
const unsigned long CALIBRATION_AVERAGE_WINDOW_MS = 6000;

const uint8_t PULSES_PER_REVOLUTION = 2;
const float RUNNING_RPM_THRESHOLD = 100.0f;
const uint8_t RUNNING_SPINUP_PERCENT = 30;

const uint8_t STARTUP_CANDIDATES[] = {0, 5, 8, 10, 12, 15, 18, 20};
const uint8_t RUNNING_CANDIDATES[] = {20, 18, 15, 12, 10, 8, 5, 0};
const uint8_t CALIBRATION_STEPS[] = {
  0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50,
  55, 60, 65, 70, 75, 80, 85, 90, 95, 100
};

const uint8_t STARTUP_CANDIDATE_COUNT = sizeof(STARTUP_CANDIDATES) / sizeof(STARTUP_CANDIDATES[0]);
const uint8_t RUNNING_CANDIDATE_COUNT = sizeof(RUNNING_CANDIDATES) / sizeof(RUNNING_CANDIDATES[0]);
const uint8_t CALIBRATION_STEP_COUNT = sizeof(CALIBRATION_STEPS) / sizeof(CALIBRATION_STEPS[0]);

volatile unsigned long fan1PulseCount = 0;
volatile unsigned long fan2PulseCount = 0;

enum TestPhase {
  PHASE_STARTUP_SPINDOWN,
  PHASE_STARTUP_EVALUATE,
  PHASE_RUNNING_SPINUP,
  PHASE_RUNNING_EVALUATE,
  PHASE_CALIBRATION_EVALUATE,
  PHASE_DONE,
  PHASE_FATAL_ERROR
};

struct RpmStats {
  uint16_t sampleCount;
  float fan1Sum;
  float fan2Sum;
  float fan1Min;
  float fan1Max;
  float fan2Min;
  float fan2Max;
};

TestPhase currentPhase = PHASE_STARTUP_SPINDOWN;
uint8_t currentIndex = 0;
uint8_t currentEffectivePercent = 0;
uint8_t currentArduinoPwm = 255;
unsigned long phaseStartedMs = 0;
unsigned long lastReportMs = 0;
unsigned long lastFan1PulseCount = 0;
unsigned long lastFan2PulseCount = 0;
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

void applyEffectiveFanPercent(uint8_t effectivePercent) {
  currentEffectivePercent = effectivePercent;
  currentArduinoPwm = effectivePercentToArduinoPwm(effectivePercent);
  analogWrite(PIN_FAN_PWM, currentArduinoPwm);
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

void printPhaseName(TestPhase phase) {
  switch (phase) {
    case PHASE_STARTUP_SPINDOWN:
      Serial.print(F("STARTUP_SPINDOWN"));
      break;
    case PHASE_STARTUP_EVALUATE:
      Serial.print(F("STARTUP_EVALUATE"));
      break;
    case PHASE_RUNNING_SPINUP:
      Serial.print(F("RUNNING_SPINUP"));
      break;
    case PHASE_RUNNING_EVALUATE:
      Serial.print(F("RUNNING_EVALUATE"));
      break;
    case PHASE_CALIBRATION_EVALUATE:
      Serial.print(F("CALIBRATION_EVALUATE"));
      break;
    case PHASE_DONE:
      Serial.print(F("DONE"));
      break;
    case PHASE_FATAL_ERROR:
      Serial.print(F("FATAL_ERROR"));
      break;
  }
}

void printFloatCsv(float value) {
  Serial.print(value, 1);
}

void printResultBool(bool value) {
  Serial.print(value ? F("1") : F("0"));
}

void printStartupBanner() {
  Serial.println();
  Serial.println(F("AutoTerra manual hardware test: fan PWM calibration"));
  Serial.println(F("Target board: Arduino Nano 33 IoT"));
  Serial.println(F("PWM pin: D6 shared fan PWM driver through inverting 2N3904 stage"));
  Serial.println(F("Tach pins: D9 fan 1 tach, D10 fan 2 tach"));
  Serial.println(F("This calibration test uses Arduino analogWrite() only."));
  Serial.println(F("It does not validate the final approximately 25 kHz PWM requirement."));
  Serial.println(F("WARNING: D6 uses an inverting transistor driver."));
  Serial.println(F("WARNING: The fan PWM collector node is pulled up to 5 V and must not connect directly to a Nano GPIO."));
  Serial.println(F("WARNING: D9 and D10 require external 10 kOhm pull-ups to 3.3 V. Internal pull-ups are not enabled."));
  Serial.println(F("WARNING: Tach outputs must remain separate and must not be tied together."));
  Serial.println(F("RPM calculation assumes two tach pulses per revolution unless hardware testing proves otherwise."));
  Serial.println();
  Serial.println(F("STATUS,phase,percent,pwm,elapsed_ms,fan1_pulses,fan1_rpm,fan2_pulses,fan2_rpm,used_for_summary"));
  Serial.println(F("STARTUP_RESULT,percent,pwm,fan1_avg_rpm,fan2_avg_rpm,fan1_started,fan2_started"));
  Serial.println(F("RUNNING_RESULT,percent,pwm,fan1_avg_rpm,fan2_avg_rpm,fan1_running,fan2_running"));
  Serial.println(F("CAL_RESULT,percent,pwm,fan1_avg_rpm,fan2_avg_rpm,fan1_min_rpm,fan1_max_rpm,fan2_min_rpm,fan2_max_rpm"));
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

void beginPhase(TestPhase nextPhase, uint8_t index, uint8_t effectivePercent, unsigned long nowMs) {
  currentPhase = nextPhase;
  currentIndex = index;
  applyEffectiveFanPercent(effectivePercent);
  resetStats();

  phaseStartedMs = nowMs;
  lastReportMs = nowMs;
  lastFan1PulseCount = readFan1Pulses();
  lastFan2PulseCount = readFan2Pulses();

  Serial.println();
  Serial.print(F("PHASE_BEGIN,"));
  printPhaseName(currentPhase);
  Serial.print(F(",percent="));
  Serial.print(currentEffectivePercent);
  Serial.print(F(",pwm="));
  Serial.println(currentArduinoPwm);
}

bool currentSampleIsUsedForSummary(unsigned long elapsedMs) {
  switch (currentPhase) {
    case PHASE_STARTUP_EVALUATE:
    case PHASE_RUNNING_EVALUATE:
      return true;
    case PHASE_CALIBRATION_EVALUATE:
      return elapsedMs >= (CALIBRATION_STEP_MS - CALIBRATION_AVERAGE_WINDOW_MS);
    default:
      return false;
  }
}

void printStatusReport(unsigned long nowMs) {
  unsigned long intervalMs = nowMs - lastReportMs;
  unsigned long elapsedMs = nowMs - phaseStartedMs;

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
  printPhaseName(currentPhase);
  Serial.print(F(","));
  Serial.print(currentEffectivePercent);
  Serial.print(F(","));
  Serial.print(currentArduinoPwm);
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
  printResultBool(usedForSummary);
  Serial.println();

  lastReportMs = nowMs;
  lastFan1PulseCount = fan1Pulses;
  lastFan2PulseCount = fan2Pulses;
}

void printStartupSummary() {
  float fan1Avg = averageFan1Rpm();
  float fan2Avg = averageFan2Rpm();

  Serial.print(F("STARTUP_RESULT,"));
  Serial.print(currentEffectivePercent);
  Serial.print(F(","));
  Serial.print(currentArduinoPwm);
  Serial.print(F(","));
  printFloatCsv(fan1Avg);
  Serial.print(F(","));
  printFloatCsv(fan2Avg);
  Serial.print(F(","));
  printResultBool(fan1Avg > RUNNING_RPM_THRESHOLD);
  Serial.print(F(","));
  printResultBool(fan2Avg > RUNNING_RPM_THRESHOLD);
  Serial.println();
}

void printRunningSummary() {
  float fan1Avg = averageFan1Rpm();
  float fan2Avg = averageFan2Rpm();

  Serial.print(F("RUNNING_RESULT,"));
  Serial.print(currentEffectivePercent);
  Serial.print(F(","));
  Serial.print(currentArduinoPwm);
  Serial.print(F(","));
  printFloatCsv(fan1Avg);
  Serial.print(F(","));
  printFloatCsv(fan2Avg);
  Serial.print(F(","));
  printResultBool(fan1Avg > RUNNING_RPM_THRESHOLD);
  Serial.print(F(","));
  printResultBool(fan2Avg > RUNNING_RPM_THRESHOLD);
  Serial.println();
}

void printCalibrationSummary() {
  Serial.print(F("CAL_RESULT,"));
  Serial.print(currentEffectivePercent);
  Serial.print(F(","));
  Serial.print(currentArduinoPwm);
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
  Serial.println();
}

void advanceState(unsigned long nowMs) {
  unsigned long elapsedMs = nowMs - phaseStartedMs;

  switch (currentPhase) {
    case PHASE_STARTUP_SPINDOWN:
      if (elapsedMs >= STARTUP_SPINDOWN_MS) {
        beginPhase(PHASE_STARTUP_EVALUATE, currentIndex, STARTUP_CANDIDATES[currentIndex], nowMs);
      }
      break;

    case PHASE_STARTUP_EVALUATE:
      if (elapsedMs >= STARTUP_EVALUATION_MS) {
        printStartupSummary();
        uint8_t nextIndex = currentIndex + 1;
        if (nextIndex < STARTUP_CANDIDATE_COUNT) {
          beginPhase(PHASE_STARTUP_SPINDOWN, nextIndex, 0, nowMs);
        } else {
          beginPhase(PHASE_RUNNING_SPINUP, 0, RUNNING_SPINUP_PERCENT, nowMs);
        }
      }
      break;

    case PHASE_RUNNING_SPINUP:
      if (elapsedMs >= RUNNING_SPINUP_MS) {
        beginPhase(PHASE_RUNNING_EVALUATE, currentIndex, RUNNING_CANDIDATES[currentIndex], nowMs);
      }
      break;

    case PHASE_RUNNING_EVALUATE:
      if (elapsedMs >= RUNNING_EVALUATION_MS) {
        printRunningSummary();
        uint8_t nextIndex = currentIndex + 1;
        if (nextIndex < RUNNING_CANDIDATE_COUNT) {
          beginPhase(PHASE_RUNNING_EVALUATE, nextIndex, RUNNING_CANDIDATES[nextIndex], nowMs);
        } else {
          beginPhase(PHASE_CALIBRATION_EVALUATE, 0, CALIBRATION_STEPS[0], nowMs);
        }
      }
      break;

    case PHASE_CALIBRATION_EVALUATE:
      if (elapsedMs >= CALIBRATION_STEP_MS) {
        printCalibrationSummary();
        uint8_t nextIndex = currentIndex + 1;
        if (nextIndex < CALIBRATION_STEP_COUNT) {
          beginPhase(PHASE_CALIBRATION_EVALUATE, nextIndex, CALIBRATION_STEPS[nextIndex], nowMs);
        } else {
          beginPhase(PHASE_DONE, 0, 0, nowMs);
          Serial.println(F("TEST_COMPLETE,fan-pwm-calibration"));
          Serial.println(F("Final 25 kHz PWM validation remains open."));
        }
      }
      break;

    case PHASE_DONE:
    case PHASE_FATAL_ERROR:
      break;
  }
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

  if (!configureTachInterrupts()) {
    currentPhase = PHASE_FATAL_ERROR;
    applyEffectiveFanPercent(0);
    return;
  }

  unsigned long nowMs = millis();
  beginPhase(PHASE_STARTUP_SPINDOWN, 0, 0, nowMs);
}

void loop() {
  if (currentPhase == PHASE_FATAL_ERROR) {
    return;
  }

  unsigned long nowMs = millis();

  if (currentPhase != PHASE_DONE && (nowMs - lastReportMs >= REPORT_INTERVAL_MS)) {
    printStatusReport(nowMs);
  }

  advanceState(nowMs);
}
