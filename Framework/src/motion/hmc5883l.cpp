/*
 * File:   hmc5883l.cpp
 * Author: Steven Noppe
 * Modifications for HROS1-Framework: Charles Wood
 *
 * Revision date: December 3, 2017
 */

#ifndef _HMC5883L_C_
#define _HMC5883L_C_

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

#include "hmc5883l.h"

using namespace Robot;

HMC5883L* HMC5883L::m_UniqueInstance = new HMC5883L();

HMC5883L::HMC5883L() :
    // Default scaling values based on calibration done on Dec 3, 2017
    minX(-566),
    maxX( 651),
    minY(-685),
    maxY( 526),
    minZ(-447),
    maxZ( 684)
{
}

HMC5883L::~HMC5883L()
{
}

void HMC5883L::selectDevice(int fd, int addr, char *name) {
    // initialize the HMC5883lL as an I2C slave
    if (ioctl(fd, I2C_SLAVE, addr) < 0) {
        fprintf(stderr, "%s not present\n", name);
        exit(1) ;
    }
}

void HMC5883L::writeToDevice(int fd, int reg, int val) {
    char buf[2];
    buf[0]=reg;
    buf[1]=val;

    if (write(fd, buf, 2) != 2) {
        fprintf(stderr, "Can't write to HMC5883L\n");
        exit(1);
    }
}

// Getters for private x/y/z vars
short HMC5883L::getX() {
    return x;
}
  
short HMC5883L::getY() {
    return y;
}
  
short HMC5883L::getZ() {
    return z;
}

float HMC5883L::getAngle() {
    return atan2(y, x) * 180 / M_PI;
}

float HMC5883L::getScaledAngle() {
    return atan2(scaledY, scaledX) * 180 / M_PI;
}

float HMC5883L::getHeading() {
    return heading;
}

float HMC5883L::getHeadingDegrees() {
    return heading * 180/M_PI;
}

float HMC5883L::getHeadingMinutes() {
    float headingDegrees = getHeadingDegrees();
    float degrees = floor(headingDegrees);
    return round((headingDegrees - degrees) * 60);
}

void HMC5883L::initializeHMC5883L() {
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
void HMC5883L::updateActualHeading(float roll, float pitch) {
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

void HMC5883L::updateData() {
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
void HMC5883L::calibrate() {
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
//
//void HMC5883L::LoadINISettings(minIni* ini) {
//    LoadINISettings(ini, HMC5883L_SECTION);
//}
//
//void HMC5883L::LoadINISettings(minIni* ini, const std::string &section) {
//    double value = INVALID_VALUE;
//
//    if ((value = ini->getd(section, "minX", INVALID_VALUE)) != INVALID_VALUE)
//        minX = value;
//    if ((value = ini->getd(section, "maxX", INVALID_VALUE)) != INVALID_VALUE)
//        maxX = value;
//
//    if ((value = ini->getd(section, "minY", INVALID_VALUE)) != INVALID_VALUE)
//        minY = value;
//    if ((value = ini->getd(section, "maxY", INVALID_VALUE)) != INVALID_VALUE)
//        maxY = value;
//
//    if ((value = ini->getd(section, "minZ", INVALID_VALUE)) != INVALID_VALUE)
//        minZ = value;
//    if ((value = ini->getd(section, "maxZ", INVALID_VALUE)) != INVALID_VALUE)
//        maxZ = value;
//}
//
//void HMC5883L::SaveINISettings(minIni* ini) {
//    SaveINISettings(ini, HMC5883L_SECTION);
//}
//
//void HMC5883L::SaveINISettings(minIni* ini, const std::string &section) {
//    ini->put(section, "minX", minX);
//    ini->put(section, "maxX", maxX);
//
//    ini->put(section, "minY", minY);
//    ini->put(section, "maxY", maxY);
//
//    ini->put(section, "minZ", minZ);
//    ini->put(section, "maxZ", maxZ);
//}



#endif //_HMC5883L_C_