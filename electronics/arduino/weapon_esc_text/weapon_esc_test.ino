#include <ESP32Servo.h>

// Use the actual ESP32 GPIO connected to the ESC signal wire.
// D9 may work on some boards, but explicit GPIO numbers are safer.
const int WEAPON_ESC_PIN = D9;

// BLHeli-style throttle pulse widths
const int THROTTLE_STOP = 1000;   // minimum throttle / stopped
const int THROTTLE_MAX  = 2000;   // full throttle

// Conservative weapon test throttle values
const int THROTTLE_LOW  = 1100;
const int THROTTLE_MED  = 1250;
const int THROTTLE_HIGH = 1400;   // keep low for bench testing

const int STARTUP_DELAY_MS = 3000;
const int ARM_DELAY_MS = 3000;
const int STEP_DELAY_MS = 2000;
const int RAMP_STEP_US = 10;
const int RAMP_STEP_DELAY_MS = 30;

Servo weaponESC;
bool escArmed = false;
int currentThrottle = THROTTLE_STOP;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Weapon ESC Test...");

  ESP32PWM::allocateTimer(0);

  delay(2000);

  armWeaponESC();
  printCommands();
}

void armWeaponESC() {
  Serial.println("========== WEAPON ESC ARMING ==========");

  weaponESC.attach(WEAPON_ESC_PIN, THROTTLE_STOP, THROTTLE_MAX);
  Serial.println("ESC attached");

  Serial.println("Sending minimum throttle / stop...");
  weaponESC.writeMicroseconds(THROTTLE_STOP);
  currentThrottle = THROTTLE_STOP;

  Serial.println("Waiting for ESC to arm...");
  delay(STARTUP_DELAY_MS + ARM_DELAY_MS);

  escArmed = true;
  Serial.println("Weapon ESC armed.");
  Serial.println("=======================================");
}

void setWeaponThrottle(int throttleUs) {
  if (!escArmed) {
    Serial.println("ESC is not armed. Command ignored.");
    return;
  }

  throttleUs = constrain(throttleUs, THROTTLE_STOP, THROTTLE_MAX);
  weaponESC.writeMicroseconds(throttleUs);
  currentThrottle = throttleUs;

  Serial.printf("Weapon throttle set to %d us\n", throttleUs);
}

void stopWeapon() {
  weaponESC.writeMicroseconds(THROTTLE_STOP);
  currentThrottle = THROTTLE_STOP;
  Serial.println("WEAPON STOPPED");
}

void rampWeaponTo(int targetThrottle) {
  if (!escArmed) {
    Serial.println("ESC is not armed. Ramp ignored.");
    return;
  }

  targetThrottle = constrain(targetThrottle, THROTTLE_STOP, THROTTLE_MAX);

  Serial.printf("Ramping weapon from %d us to %d us\n", currentThrottle, targetThrottle);

  if (targetThrottle > currentThrottle) {
    for (int t = currentThrottle; t <= targetThrottle; t += RAMP_STEP_US) {
      weaponESC.writeMicroseconds(t);
      currentThrottle = t;
      delay(RAMP_STEP_DELAY_MS);
    }
  } else {
    for (int t = currentThrottle; t >= targetThrottle; t -= RAMP_STEP_US) {
      weaponESC.writeMicroseconds(t);
      currentThrottle = t;
      delay(RAMP_STEP_DELAY_MS);
    }
  }

  weaponESC.writeMicroseconds(targetThrottle);
  currentThrottle = targetThrottle;
  Serial.printf("Ramp complete: %d us\n", currentThrottle);
}

void runWeaponTest() {
  if (!escArmed) {
    Serial.println("Cannot run test. ESC is not armed.");
    return;
  }

  Serial.println("========== WEAPON SPIN TEST ==========");
  Serial.println("Make sure the robot is restrained and the weapon is safe.");

  Serial.println("Step 1: stopped");
  stopWeapon();
  delay(STEP_DELAY_MS);

  Serial.println("Step 2: low throttle");
  rampWeaponTo(THROTTLE_LOW);
  delay(STEP_DELAY_MS);

  Serial.println("Step 3: medium throttle");
  rampWeaponTo(THROTTLE_MED);
  delay(STEP_DELAY_MS);

  Serial.println("Step 4: conservative high throttle");
  rampWeaponTo(THROTTLE_HIGH);
  delay(STEP_DELAY_MS);

  Serial.println("Step 5: ramp down and stop");
  rampWeaponTo(THROTTLE_STOP);
  stopWeapon();

  Serial.println("Weapon test complete.");
  Serial.println("======================================");
}

void printCommands() {
  Serial.println();
  Serial.println("AVAILABLE COMMANDS");
  Serial.println("==================");
  Serial.println("'t' - Run weapon spin test");
  Serial.println("'0' - Stop weapon");
  Serial.println("'1' - Low throttle");
  Serial.println("'2' - Medium throttle");
  Serial.println("'3' - Conservative high throttle");
  Serial.println("'r' - Ramp down to stop");
  Serial.println("'a' - Re-arm ESC");
  Serial.println("'h' - Help");
  Serial.println("==================");
  Serial.println();
}

void processSerialCommand() {
  if (Serial.available() <= 0) {
    return;
  }

  char command = Serial.read();

  switch (command) {
    case 't':
      runWeaponTest();
      break;

    case '0':
    case 's':
      stopWeapon();
      break;

    case '1':
      rampWeaponTo(THROTTLE_LOW);
      break;

    case '2':
      rampWeaponTo(THROTTLE_MED);
      break;

    case '3':
      rampWeaponTo(THROTTLE_HIGH);
      break;

    case 'r':
      rampWeaponTo(THROTTLE_STOP);
      stopWeapon();
      break;

    case 'a':
      Serial.println("Re-arming weapon ESC...");
      escArmed = false;
      stopWeapon();
      armWeaponESC();
      break;

    case 'h':
      printCommands();
      break;

    case '\n':
    case '\r':
      break;

    default:
      Serial.println("Unknown command. Type 'h' for help.");
      break;
  }
}

void loop() {
  processSerialCommand();
  delay(20);
}
