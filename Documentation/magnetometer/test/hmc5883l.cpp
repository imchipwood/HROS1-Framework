/*
 * Using the HMC5883L (GY-273)
 *
 * Connection on a Raspberry Pi 3 model B v1.2 :
 * - VCC -> 3V3 (1)
 * - GND -> GND (6)
 * - SCL -> GPIO3 - SCL1 I2C (5)
 * - SDA -> GPIO2 - SDA1 I2C (3)
 *
 * Compiled in Netbeans IDE 8.1
 * Also possible to compile with GCC :
 * 'GCC main.cpp -o HMC5883L -lm'
 */

/* 
 * File:   main.cpp
 * Author: Steven Noppe
 * Modifications for HROS1-Framework: Charles Wood
 *
 * Revision date : December 3, 2017
 */

#ifndef HMC5883L_C
#define HMC5883L_C

/*
 * Compiling with GCC, gives a fault because M_PI is not defined
 * With an IDE like Netbeans, we do not have that problem
 * Thats why we define it here
 */
#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

/*
 * Same problem here with GCC, it seems he doesn't know what 'true' is 
 */
#ifndef true
    #define true 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/i2c-dev.h>
#include <math.h>
#include <iostream>
#include <fstream>


class HMC5883L
{
    private:
        short x;
        short y;
        short z;
        float scaledX;
        float scaledY;
        float scaledZ;

        float heading; // radians
        float minutes;
        float headingX; // radians
        float headingY; // radians

        // Default scaling values based on calibration done on Dec 3, 2017
        short minX = -509;
        short maxX =  679;
        short minY = -743;
        short maxY =  437;
        short minZ = -261;
        short maxZ =  763;

        float angle;
  
        int fd;
        unsigned char buf[16];

        /*
         * Chances are, your HCM5883L is address 0x1E. Double check with this command:
         * sudo i2cdetect -y 1
         */
        const int HMC5883L_I2C_ADDR = 0x1E;

        void selectDevice(int fd, int addr, char *name) {
            // initialize the HMC5883lL as an I2C slave
            if (ioctl(fd, I2C_SLAVE, addr) < 0) {
                fprintf(stderr, "%s not present\n", name);
                exit(1) ;
            }
        }

        /*
         * I2C write
         */
        void writeToDevice(int fd, int reg, int val) {
            char buf[2];
            buf[0]=reg;
            buf[1]=val;

            if (write(fd, buf, 2) != 2) {
                fprintf(stderr, "Can't write to HMC5883L\n");
                exit(1);
            }
        }
        
  
    public:
        // Getters for private x/y/z vars
        short getX() {
            return x;
        }
  
        short getY() {
            return y;
        }
  
        short getZ() {
            return z;
        }

        float getAngle() {
            return atan2(y, x) * 180 / M_PI;
        }

        float getScaledAngle() {
            return atan2(scaledY, scaledX) * 180 / M_PI;
        }

        float getHeading() {
            return heading;
        }

        float getHeadingDegrees() {
            return heading * 180/M_PI;
        }

        float getHeadingMinutes() {
            float headingDegrees = getHeadingDegrees();
            float degrees = floor(headingDegrees);
            return round((headingDegrees - degrees) * 60);
        }
  
        void initializeHMC5883L() {
            /*
             * https://en.wikipedia.org/wiki/Open_(system_call)
             * Attempt to open /dev/i2c-1
             * Returns a negative number if unsuccessful
             */
            if ((fd = open("/dev/i2c-1", O_RDWR)) < 0) {
                fprintf(stderr, "Failed to open i2c bus\n");
                exit(1);
            }
 
            // Init device
            selectDevice(fd, HMC5883L_I2C_ADDR, "HMC5883L");

            /*
             * Config Register B - addr 0x01
             * Gain control
             * Set to 0x20 for:
             *      820 LSb/Gauss
             *      1.22 mG/LSb
             */
            writeToDevice(fd, 0x01, 0x20); // 0x01 = configuration register A

            /*
             * Mode Register - addr 0x02
             * Set to 0x00 for continuous measurement mode
             */
            writeToDevice(fd, 0x02, 0x00);
        }

        /*
         * "heading" is only calculated using the X & Y axes...
         * That is unfortunately useless if the sensor is tilted at all.
         * updateActualHeading will calculate the actual heading based on
         * roll/pitch input from an accelerometer and/or gyro in RADIANS
         */
        void updateActualHeading(float roll, float pitch) {
            // https://arduino.stackexchange.com/questions/19359/compute-yaw-from-magnetometer-and-accelerometer
            const float cosRoll =  cos(roll);
            const float sinRoll =  sin(roll);
            const float cosPitch = cos(pitch);
            const float sinPitch = sin(pitch);

            const float magX = (float)scaledX * cosPitch +
                               (float)scaledY * sinRoll * sinPitch +
                               (float)scaledZ * cosRoll * sinPitch;
                    
            const float magY = (float)scaledY * cosRoll -
                               (float)scaledZ * sinRoll;

            const float tmp  = sqrt(magX * magX + magY * magY);
            
            headingX = magX / tmp;
            headingY = -magY / tmp;
        }

        float getHeadingX() {
            return headingX;
        }

        float getHeadingY() {
            return headingY;
        }
  
        void updateData() {
            buf[0] = 0x03;

            if ((write(fd, buf, 1)) != 1) {
                // Send the register to read from
                fprintf(stderr, "Error writing to i2c slave\n");
            }

            if (read(fd, buf, 6) != 6) {
                fprintf(stderr, "Unable to read from HMC5883L\n");
            } else {
                // Received the values
                x = (buf[0] << 8) | buf[1];
                z = (buf[2] << 8) | buf[3];
                y = (buf[4] << 8) | buf[5];

                // apply bias
                x -= (minX + maxX) / 2;
                y -= (minY + maxY) / 2;
                z -= (minZ + maxZ) / 2;

                // scale based on min/max values
                scaledX = (float)(x - minX) / (float)(maxX - minX) * 2 - 1;
                scaledY = (float)(y - minY) / (float)(maxY - minY) * 2 - 1;
                scaledZ = (float)(z - minZ) / (float)(maxZ - minZ) * 2 - 1;
        
                float declDegree = 0;
                float declMin = 35;
                float decl = (declDegree+declMin/60) * (M_PI/180);

                //float headingRad = atan2(y, x);
                float headingRad = atan2(scaledY, scaledX);
                headingRad += decl;
        
                if (headingRad < 0)
                    headingRad += 2*M_PI;
        
                if (headingRad > 2*M_PI)
                    headingRad -= 2*M_PI;

                heading = headingRad;
            }
        }

        /*
         * Calibrate
         *
         * Collects magnetometer readings for 15 seconds in order to re-calculate the min/max values for each axis
         *
         * Magnetometer readings are not necessarily centered around 0 - calibration is required to normalize values
         * to the magnetic field wherever the magnetometer is being used
         *
         *
         */
        void calibrate() {
            minX = 0;
            maxX = 0;
            minY = 0;
            maxY = 0;
            minZ = 0;
            maxZ = 0;
            int secondsToCalibrate = 15;
            secondsToCalibrate *= 10;
            printf("Rotate the mag along all of its axes multiple times\n");
            printf("Press enter to begin the calibration");
            getchar();
            while (secondsToCalibrate >= 0) {
                secondsToCalibrate--;
                updateData();

                // update min/max values
                if (x < minX)
                    minX = x;
                if (x > maxX)
                    maxX = x;

                if (y < minY)
                    minY = y;
                if (y > maxY)
                    maxY = y;

                if (z < minZ)
                    minZ = z;
                if (z > maxZ)
                    maxZ = z;

                // sleep 100mS
                usleep(100 * 1000);
            }
            printf("min/max values\n");
            printf("x: %d, %d\n", minX, maxX);
            printf("y: %d, %d\n", minY, maxY);
            printf("z: %d, %d\n", minZ, maxZ);
        }
};

// Main program
int main(int argc, char **argv)
{
    HMC5883L compass;
 
    compass.initializeHMC5883L();

    // calibration -
    //compass.calibrate();
         
    FILE *fp;
    fp = fopen("dump.csv", "w");
    fprintf(fp, "x,y,z\n");
    fflush(fp);

    while (true)
    {
        compass.updateData();
        compass.updateActualHeading(0.0, 0.0);
  
        printf("degrees: %0.2f, minutes: %0.2f, angle: %f\n", 
            compass.getHeadingDegrees(), compass.getHeadingMinutes(), compass.getScaledAngle());
        //printf("degrees: %0.2f, minutes: %0.2f, angle: %f\n",
        //    compass.getHeadingDegrees(), compass.getHeadingMinutes(), compass.getAngle());
        //printf("x, y: %f, %f\n", compass.getHeadingX(), compass.getHeadingY());
        fflush(stdout);

        // write data to file for
        fprintf(fp, "%d,%d,%d\n", compass.getX(), compass.getY(), compass.getZ());
        fflush(fp);

        usleep(500 * 1000);
    }
    fclose(fp);
    return 0;
}

#endif //HMC5883L_C