
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../../../Framework/include/hmc5883l.h"

// Main program

using namespace Robot;

int main(int argc, char **argv)
{
    HMC5883L compass;

    compass.initializeHMC5883L();

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
