#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

/* Assign a unique ID to this sensor at the same time */
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

/* Duration, in ms, it takes to deploy or retract the fins. */
static int FIN_EXTEND_DURATION = 400;
/* Vertical acceleration, in m/s^2, above which to consider the motor to be ignited. */
static float MOTOR_IGNITE_ACCEL = 30.0;
/* Vertical acceleration, in m/s^2, below which to consider the motor to be burned out. */
static float MOTOR_BURNOUT_ACCEL = 0.0;
/* Delay after burnout, in ms, to wait to deploy the fins. */
static int DEPLOYMENT_DELAY = 3000;
/* Net acceleration, in m/s^2, above which to consider the rocket to be at apogee. */
static float APOGEE_ACCEL = 0.0;

static int BUZZER_PIN = 12;
static int MOTOR_L_PIN = 11;
static int MOTOR_W_PIN = 10;
static int LED_PIN = 13;

/* Are the fins deployed? */
bool finsDeployed = false;

/* What stage is the flight in?
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

void setup(void) 
{
  /* Initialize the output pins */
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(MOTOR_L_PIN, OUTPUT);
  pinMode(MOTOR_W_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  signalPower();

#ifndef ESP8266
  while (!Serial); // for Leonardo/Micro/Zero
#endif
  Serial.begin(9600);

  Serial.println("Beginning program.");
  
  /* Initialise the sensor */
  if(!accel.begin())
  {
    Serial.print("Failed to initialize altimeter!");
  }

  Serial.println("Beginning startup sequence.");
  accel.setRange(ADXL345_RANGE_16_G);
  flexFins();
  signalStartupSuccess();
}

float calcVectLen(float x, float y, float z)
{
  return sqrt(x*x + y*y + z*z);
}

void deployFinsIfUndeployed(void)
{
  if(!finsDeployed) {
    startMotor(true);
    delay(FIN_EXTEND_DURATION);
    stopMotor();
    finsDeployed = true;
  }
}

void retractFinsIfDeployed(void)
{
   if(finsDeployed) {
    startMotor(false);
    delay(FIN_EXTEND_DURATION);
    stopMotor();
    finsDeployed = false;
  }
}

void signalStartupSuccess(void)
{
  for(int i = 0; i <= 4; i++){
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);
    delay(250*i);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    delay(250*i);
  }
}

void signalPower(void)
{
  for(int i = 0; i <= 3; i++){
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
}

void flexFins(void)
{
  if(finsDeployed) {
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

void stopMotor(void) {
  digitalWrite(MOTOR_L_PIN, HIGH);
  digitalWrite(MOTOR_W_PIN, HIGH);
}

void startMotor(bool forward) {
  if (!forward) {
    digitalWrite(MOTOR_W_PIN, LOW);
    digitalWrite(MOTOR_L_PIN, HIGH);
  } else {
    digitalWrite(MOTOR_W_PIN, HIGH);
    digitalWrite(MOTOR_L_PIN, LOW);
  }
}

void loop(void) 
{
  sensors_event_t event;
  accel.getEvent(&event);

  float verticalAccel = -1.0*event.acceleration.y;

  Serial.println(flightStage);

  switch(flightStage) {
    case 0:
      // Hasn't launched yet
      if(verticalAccel >= MOTOR_IGNITE_ACCEL) {
        flightStage++;
      }
      break;
    case 1:
      // Has launched, motor is burning
      loopsSinceLaunch++;
      if(verticalAccel <= MOTOR_BURNOUT_ACCEL || loopsSinceLaunch > 16) {
        flightStage++;
        delay(DEPLOYMENT_DELAY);
      }
      break;
    case 2:
      // Motor has burnt out; is coasting to apogee
      deployFinsIfUndeployed();
      loopsSinceDeployment++;
      if(loopsSinceDeployment > 60 && verticalAccel >= APOGEE_ACCEL || loopsSinceDeployment > 120) {
        flightStage++;
      }
      break;
    case 3:
      retractFinsIfDeployed();
      while(true);
      break;
  }

  delay(250);
}
