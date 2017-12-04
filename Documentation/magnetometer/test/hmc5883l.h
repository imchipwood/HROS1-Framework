
#ifndef _HMC5883L_H_
#define _HMC5883L_H_

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#ifndef true
    #define true 1
#endif

#ifndef HMC5883L_I2C_ADDR
    #define HMC5883L_I2C_ADDR 0x1E
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

namespace Robot
{
    class HMC5883L {

        public:
            short getX();
            short getY();
            short getZ();

            float getAngle();
            float getScaledAngle();
            float getHeading();
            float getHeadingDegrees();
            float getHeadingMinutes();

            void initializeHMC5883L();

            void updateActualHeading(float roll, float pitch);

            float getheading() { return headingX; }
            float getHeadingY() { return headingY; }

            virtual ~HMC5883L();

            static HMC5883L* GetInstance() { return m_UniqueInstance; }

            void updateData();
            void calibrate();

        private:
            static HMC5883L* m_UniqueInstance;

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

            short minX = -566;
            short maxX =  651;
            short minY = -685;
            short maxY =  526;
            short minZ = -447;
            short maxZ =  684;

            float angle;

            int fd;
            unsigned char buf[16];

            void selectDevice(int fd, int addr, char *name);
            void writeToDevice(int fd, int reg, int val);

    };
}

#endif //_HMC5883L_H_