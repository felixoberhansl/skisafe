// The SFE_LSM9DS1 library requires both Wire and SPI be
// included BEFORE including the 9DS1 library.
#include <Wire.h>
#include <SPI.h>
#include <SparkFunLSM9DS1.h>

// LSM9DS1 Library Init:
// SDO_XM and SDO_G are both pulled high, so our addresses are:
#define LSM9DS1_M	0x1E // Would be 0x1C if SDO_M is LOW
#define LSM9DS1_AG	0x6B // Would be 0x6A if SDO_AG is LOW
// Use the LSM9DS1 class to create an object. [imu] can be
// named anything, we'll refer to that throught the sketch.
LSM9DS1 imu;

// Earth's magnetic field varies by location. Add or subtract
// a declination to get a more accurate heading. Calculate
// your's here:
// http://www.ngdc.noaa.gov/geomag-web/#declination
#define DECLINATION -8.58 // Declination (degrees) in Boulder, CO.

enum State {Idle, EventDetected, EventEnded, CheckIsDue};
enum State state = Idle;

#define THR_EVENT_START 0.15
#define THR_EVENT_ENDED 0.02
#define AFTER_EVENT_DELAY 2000
#define RADIO_PACKET_INTERVAL 20000
#define THR_AXIS_TILT 0.2

#define HIST_LENGTH 100
float accelHist[HIST_LENGTH];
int accelHistIdx = 0;
float axisHist[HIST_LENGTH];
int axisHistIdx = 0;

unsigned long lastRadioPacketMillis;
unsigned long eventEndedMillis;

float variance;
float axisMean;
float axisBefore;

#define VERTICAL_AXIS ay

void setup()
{

    Serial.begin(115200);

    // Before initializing the IMU, there are a few settings
    // we may need to adjust. Use the settings struct to set
    // the device's communication mode and addresses:
    imu.settings.device.commInterface = IMU_MODE_I2C;
    imu.settings.device.mAddress = LSM9DS1_M;
    imu.settings.device.agAddress = LSM9DS1_AG;
    imu.settings.accel.sampleRate = 1; // = 10 Hz
    // The above lines will only take effect AFTER calling
    // imu.begin(), which verifies communication with the IMU
    // and turns it on.
    if (!imu.begin())
    {
        Serial.println("Failed to communicate with LSM9DS1.");
        Serial.println("Double-check wiring.");
        Serial.println("Default settings in this sketch will " \
                "work for an out of the box LSM9DS1 " \
                "Breakout, but may need to be modified " \
                "if the board jumpers are.");
        while (1) {
        }
    }

    Serial.println("Initialised.");
}

void loop()
{
    if (getSensorValues()) {
        // switch to next state based on current state and possible triggers:
        switch (state) {
            case EventDetected:
                if (variance < THR_EVENT_ENDED) {
                    processEventEnded();
                }
                break;
            case EventEnded:
                if (millis() > eventEndedMillis + AFTER_EVENT_DELAY) {
                    processCheckIsDue();
                }
                break;
            default: // e.g. Idle
                if (variance > THR_EVENT_START) {
                    processEventDetected();
                }
                break;
        }
    }

    if (millis() > lastRadioPacketMillis + RADIO_PACKET_INTERVAL) {
        lastRadioPacketMillis = millis();
        sendRadioPacket();
    }
}

bool getSensorValues() {
    if ( imu.accelAvailable() )
    {
        // To read from the accelerometer, first call the
        // readAccel() function. When it exits, it'll update the
        // ax, ay, and az variables with the most current data.
        imu.readAccel();

        float ax = imu.calcAccel(imu.ax);
        float ay = imu.calcAccel(imu.ay);
        float az = imu.calcAccel(imu.az);
        float absAccel = sqrt(ax*ax + ay*ay + az*az);

        accelHistIdx = (accelHistIdx + 1) % HIST_LENGTH;
        accelHist[accelHistIdx] = absAccel;

        axisHistIdx = (axisHistIdx + 1) % HIST_LENGTH;
        axisHist[axisHistIdx] = VERTICAL_AXIS;

        if (accelHistIdx == (HIST_LENGTH-1)) {
            float sum = 0;
            float sumOfSquares = 0;
            for (int i = 0; i < HIST_LENGTH; ++i) {
                sum += accelHist[i];
                sumOfSquares += accelHist[i]*accelHist[i];
            }
            float mean = sum / HIST_LENGTH;
            float meanOfSquares = sumOfSquares / HIST_LENGTH;
            variance = sqrt(meanOfSquares - mean*mean);

            Serial.print("Variance = ");
            Serial.print(variance);
            Serial.println();
        }

        if (axisHistIdx == (HIST_LENGTH-1)) {
            float sum = 0;
            for (int i = 0; i < HIST_LENGTH; ++i) {
                sum += axisHist[i];
            }
            axisMean = sum / HIST_LENGTH;

            Serial.print("vertical mean = ");
            Serial.print(axisMean);
            Serial.println();
        }

        if (absAccel > 1.3) {
            Serial.print("Event! |a| = ");
            Serial.print(absAccel);
            Serial.println();
        }

        return true;
    }
    return false;
}

void processEventDetected() {
    state = EventDetected;

    axisBefore = axisMean;

    Serial.print("Event! |a| = ");
    Serial.print(accelHist[accelHistIdx]);
    Serial.println();
}

void processEventEnded() {
    state = EventEnded;
    eventEndedMillis = millis();

}

void processCheckIsDue() {
    state = CheckIsDue;

    // compare a_v and a_before.
    // trigger alarm accordingly
    if (abs(axisMean - axisBefore) > THR_AXIS_TILT) {
        Serial.println("TILT!");
    }

    state = Idle;
}

void sendRadioPacket() {

}
