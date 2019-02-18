#include <Adafruit_ADXL345_U.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

/** @brief Assign a unique ID to this sensor while initializing it.*/
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

/** @brief Duration, in ms, it takes to deploy or retract the fins. */
static int FIN_EXTEND_DURATION = 400;
/** @brief Vertical acceleration, in m/s^2, above which to consider the motor to
 * be ignited. */
static float MOTOR_IGNITE_ACCEL = 30.0;
/** @brief Vertical acceleration, in m/s^2, below which to consider the motor to
 * be burned out. */
static float MOTOR_BURNOUT_ACCEL = 0.0;
/** @brief Delay after burnout, in ms, to wait to deploy the fins. */
static int DEPLOYMENT_DELAY = 3000;
/** @brief Net acceleration, in m/s^2, above which to consider the rocket to be
 * at apogee. */
static float APOGEE_ACCEL = 0.0;

static int BUZZER_PIN = 12;
static int MOTOR_L_PIN = 11;
static int MOTOR_W_PIN = 10;
static int LED_PIN = 13;

/* Are the fins deployed? */
bool finsDeployed = false;

/** @brief What stage is the flight in?
  Values:
    0: Has not launched yet.
    1: Has launched, motor is burning.
    2: Motor has burnt out, coasting to apogee.
    3: Apogee reached, falling.
    <Further values not included because program ends at apogee.>
 */
int flightStage = 0;

// This is used as a failsafe timer
int loopsSinceLaunch = 0;
int loopsSinceDeployment = 0;

/**
 * @brief Automatically runs on startup.
 */
void setup(void) {
  /* Initialize the output pins */
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(MOTOR_L_PIN, OUTPUT);
  pinMode(MOTOR_W_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  signalPower();

#ifndef ESP8266
  while (!Serial)
    ;  // for Leonardo/Micro/Zero
#endif
  Serial.begin(9600);

  Serial.println("Beginning program.");

  /* Initialise the sensor */
  while (!accel.begin()) {
    Serial.println("Failed to initialize altimeter!");
    delay(1000);
    Serial.println("Retrying...");
  }
  accel.setRange(ADXL345_RANGE_16_G);

  Serial.println("System initalized. Beginning startup sequence.");
  flexFins();
  signalStartupSuccess();
}

/**
 * @brief Calculate the length of a vector starting at the origin and ending at
 * the given coordinates.
 *
 * @param x X-coordinate of the end of the vector.
 * @param y Y-coordinate of the end of the vector.
 * @param z Z-coordinate of the end of the vector.
 * @return float Distance (calculated with Pythagorean Theorem in 3D).
 */
float calcVectLen(float x, float y, float z) {
  return sqrt(x * x + y * y + z * z);
}

/**
 * @brief If fins not deployed, deploys fins. Else, does nothing.
 */
void deployFinsIfUndeployed(void) {
  if (!finsDeployed) {
    startMotor(true);
    delay(FIN_EXTEND_DURATION);
    stopMotor();
    finsDeployed = true;
  }
}

/**
 * @brief If fins deployed, retracts fins. Else, does nothing.
 */
void retractFinsIfDeployed(void) {
  if (finsDeployed) {
    startMotor(false);
    delay(FIN_EXTEND_DURATION);
    stopMotor();
    finsDeployed = false;
  }
}

/**
 * @brief Show output feedback for succesful startup sequence.
 *
 * Flashes LED and sounds buzzer 4 times, with linearly increasing flash length
 * and delay between.
 */
void signalStartupSuccess(void) {
  for (int i = 0; i <= 4; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);
    delay(250 * i);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    delay(250 * i);
  }
}

/**
 * @brief Show output feedback for inital power-on.
 *
 * Flashes LED and sounds buzzer 3 times.
 */
void signalPower(void) {
  for (int i = 0; i <= 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
}

/**
 * @brief Show output feedback proving fin control is working.
 *
 * Extends fins halfway, then retracts fins halfway.
 */
void flexFins(void) {
  if (finsDeployed) {
    return;
  }
  startMotor(true);
  delay(FIN_EXTEND_DURATION / 2);
  stopMotor();
  delay(FIN_EXTEND_DURATION);
  startMotor(false);
  delay(FIN_EXTEND_DURATION / 2);
  stopMotor();
}

/**
 * @brief Stop motor entirely.
 */
void stopMotor(void) {
  digitalWrite(MOTOR_L_PIN, HIGH);
  digitalWrite(MOTOR_W_PIN, HIGH);
}

/**
 * @brief Start the motor in the direction requested.
 *
 * @param forward If true, will turn on in deployment direction. Otherwise, will
 *   turn on in the retraction direction.
 */
void startMotor(bool forward) {
  if (!forward) {
    digitalWrite(MOTOR_W_PIN, LOW);
    digitalWrite(MOTOR_L_PIN, HIGH);
  } else {
    digitalWrite(MOTOR_W_PIN, HIGH);
    digitalWrite(MOTOR_L_PIN, LOW);
  }
}

/**
 * @brief Automatically loops while Arduino is running.
 */
void loop(void) {
  sensors_event_t event;
  accel.getEvent(&event);

  float verticalAccel = -1.0 * event.acceleration.y;

  Serial.println(flightStage);

  switch (flightStage) {
    case 0:
      // Hasn't launched yet
      if (verticalAccel >= MOTOR_IGNITE_ACCEL) {
        flightStage++;
      }
      break;
    case 1:
      // Has launched, motor is burning
      loopsSinceLaunch++;
      if (verticalAccel <= MOTOR_BURNOUT_ACCEL || loopsSinceLaunch > 16) {
        flightStage++;
        delay(DEPLOYMENT_DELAY);
      }
      break;
    case 2:
      // Motor has burnt out; is coasting to apogee
      deployFinsIfUndeployed();
      loopsSinceDeployment++;
      if (loopsSinceDeployment > 60 && verticalAccel >= APOGEE_ACCEL ||
          loopsSinceDeployment > 120) {
        flightStage++;
      }
      break;
    case 3:
      retractFinsIfDeployed();
      while (true)
        ;
      break;
  }

  delay(250);
}
