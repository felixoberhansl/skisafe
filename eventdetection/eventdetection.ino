// The SFE_LSM9DS1 library requires both Wire and SPI be
// included BEFORE including the 9DS1 library.
#include <Wire.h>
#include <SPI.h>
#include <SparkFunLSM9DS1.h>

//////////////////////////
// LSM9DS1 Library Init //
//////////////////////////
// Use the LSM9DS1 class to create an object. [imu] can be
// named anything, we'll refer to that throught the sketch.
LSM9DS1 imu;

///////////////////////
// Example I2C Setup //
///////////////////////
// SDO_XM and SDO_G are both pulled high, so our addresses are:
#define LSM9DS1_M	0x1E // Would be 0x1C if SDO_M is LOW
#define LSM9DS1_AG	0x6B // Would be 0x6A if SDO_AG is LOW

////////////////////////////
// Sketch Output Settings //
////////////////////////////
#define PRINT_CALCULATED
//#define PRINT_RAW


// Earth's magnetic field varies by location. Add or subtract
// a declination to get a more accurate heading. Calculate
// your's here:
// http://www.ngdc.noaa.gov/geomag-web/#declination
#define DECLINATION -8.58 // Declination (degrees) in Boulder, CO.

#define HIST_LENGTH 100
float accelHist[HIST_LENGTH];
int accelHistIdx = 0;

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
    // Update the sensor values whenever new data is available
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

        accelHist[accelHistIdx] = absAccel;
        accelHistIdx = (accelHistIdx + 1) % HIST_LENGTH;

        if ((accelHistIdx % 50) == 0) {
            float sum = 0;
            float sumOfSquares = 0;
            for (int i = 0; i < HIST_LENGTH; ++i) {
                sum += accelHist[i];
                sumOfSquares += accelHist[i]*accelHist[i];
            }
            float mean = sum / HIST_LENGTH;
            float meanOfSquares = sumOfSquares / HIST_LENGTH;
            float variance = sqrt(meanOfSquares - mean*mean);

            Serial.print("Variance = ");
            Serial.print(variance);
            Serial.println();
        }

        if (absAccel > 1.3) {
            Serial.print("Event! |a| = ");
            Serial.print(absAccel);
            Serial.println();
        }
    }
}

