// Add library includes here

// Fin adjustment step size
static float FIN_ADJ_STEP = 1.0;

static int MOTOR_CW_PIN = 1;
static int MOTOR_CCW_PIN = 1;
static int STATUS_PIN = 13;
static int INPUT_PIN = 1;

static float TARGET_APOGEE;

// Int representing different stages of flight
int flightStatus = 0;

float currentFinDeployment = 0;

struct flightData {
    float altitude;
    float velocity;
    float acceleration;
}

struct flightData getAltimeterData(void) {
    struct flightData altimeterData;
    // Read data from altimeter
    return altimeterData;
}

void adjustFins(float amount) {
    // Adjust the fins (amount can be either degrees or distance depending on
    // how we code it). Negative values retract, positive extend. Check for max
    // or min extension.
    // Update currentFinDeployment
}

void fullyRetractFins(void) {
    // Fully retract
}

float getExpectedApogee(float altitude, float velocity, float acceleration) {
    // Apogee calculations here
    return apogee;
}

int updateFlightStatus(void) {
    // Check and update flight status here
    return flightStatus;
}

void setup() {
    // Initialize comms here

    // Initialize pins here

    // Motor test
    // Data test

    // LED / buzzer startup codes
}

void loop() {
    status = updateFlightStatus();
    switch status {
        case 0:
            // On launchpad: keep beeping
            break;
        case 1:
            // During motor burn: do nothing
            break;
        case 2:
            // After motor burn
            // Could probably break this into another function
            struct flightData currentData = getAltimeterData();
            float apogee = getExpectedApogee(
                currentData.altitude, 
                currentData.velocity, 
                currentData.acceleration);
            if(apogee < TARGET_APOGEE) {
                adjustFins(-FIN_ADJ_STEP)
            } else if(apogee > TARGET_APOGEE) {
                adjustFins(FIN_ADJ_STEP)
            }
            break;
        case 3:
            // After apogee
            fullyRetractFins();
            break;
    }
    delay(500);
}