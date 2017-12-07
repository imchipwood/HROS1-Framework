
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

#define HMC5883L_SECTION "Walking Config"
#define INVALID_VALUE   -1024.0

namespace Robot
{
    class HMC5883L
    {
        private:
            static HMC5883L* m_UniqueInstance;

            short x;
            short y;
            short z;
            float scaledX;
            float scaledY;
            float scaledZ;

            short minX;
            short maxX;
            short minY;
            short maxY;
            short minZ;
            short maxZ;

            float heading; // radians
            float minutes;
            float headingX; // radians
            float headingY; // radians

            float angle;

            int fd;
            unsigned char buf[16];

            void selectDevice(int fd, int addr, char *name);
            void writeToDevice(int fd, int reg, int val);

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

            //virtual ~HMC5883L();
            ~HMC5883L();
            HMC5883L();

            static HMC5883L* GetInstance() { return m_UniqueInstance; }

            void updateData();
            void calibrate();

			//void LoadINISettings(minIni* ini);
			//void LoadINISettings(minIni* ini, const std::string &section);
			//void SaveINISettings(minIni* ini);
			//void SaveINISettings(minIni* ini, const std::string &section);

    };
}

#endif //_HMC5883L_H_