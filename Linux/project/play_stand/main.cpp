/*
	This program will open and interpret a text file to give operations
	to the HR-OS1 robot to perform. The text file format should be:
	
		<Command> <Param>
		
	where Command and Param are both integers. To perform a continuous
	sequence of operations, the text file needs more than one line of 
	commands.
	
	By default, a textfile named script.txt will automatically be read,
	but user has the option to use a differently named textfile by 
	including that file name on the commandline as an argument upon
	starting up this program.
	
	Avoid adding comments to the script/text file.
	
	Commands (param interpretation):	
	1 Walk 			(seconds to walk for)
	2 Turn			(degrees to turn)
	3 Play Motion 	(page of motion in rme)
	4 Sleep 		(milliseconds to sleep for)
	5 Play MP3		**not developed yet
	6 Send Midi		**not developed yet
	7 Exit			*no param needed. Exit program
	
	To be used for Portland Cyber Show.
	
	Updated: 10-14-2017
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <term.h>
#include <ncurses.h>
#include <signal.h>
#include <libgen.h>
#include "standup.h"
#include "mjpg_streamer.h"
#include "hmc5883l.h"
#include <iostream>
#include <fstream>
#include <math.h>

#define MOTION_FILE_PATH    "../../../Data/motion_4096.bin"
#define INI_FILE_PATH       "../../../Data/config.ini"

using namespace std;
using namespace Robot;

HMC5883L compass;

LinuxMotionTimer linuxMotionTimer;
LinuxArbotixPro linux_arbotixpro("/dev/ttyUSB0");
ArbotixPro arbotixpro(&linux_arbotixpro);
minIni* ini;
void play_script(void);


void motion(int page_num){

    printf("playing ");
    MotionManager::GetInstance()->LoadINISettings(ini);
    MotionManager::GetInstance()->SetEnable(false);
    MotionManager::GetInstance()->AddModule((MotionModule*)Action::GetInstance());
    linuxMotionTimer.Stop();
    PlayCmd(&arbotixpro, page_num);
    MotionManager::GetInstance()->RemoveModule((MotionModule*)Action::GetInstance());
}

// Function that calls correct motion to stand based on current fallen direction
void stand_up(int direction) {
    if (direction == FORWARD) {
        motion(STANDUP_FACE);
    } else if (direction == BACKWARD) {
        motion(STANDUP_BACK);
    }
}


void walk(int direction, int second){
    printf("walking...\t");
    cout << "Walking...\t";
    Walking::GetInstance()->LoadINISettings(ini);
    linuxMotionTimer.Start();
    MotionManager::GetInstance()->AddModule((MotionModule*)Walking::GetInstance());
    MotionManager::GetInstance()->SetEnable(true);
    MotionManager::GetInstance()->ResetGyroCalibration();
    Walking::GetInstance()->X_OFFSET = direction * -2;
    Walking::GetInstance()->A_MOVE_AMPLITUDE = 0;
    Walking::GetInstance()->X_MOVE_AMPLITUDE = direction * 10;
    Walking::GetInstance()->Start();

    for (int i = 0; i < second * 1000000; i = i + 10000) {
        if (MotionStatus::FALLEN != STANDUP) {
            Walking::GetInstance()->Stop();
            usleep(10000);
            cout << "Robot fallen " << MotionStatus::FALLEN << ".\n";
            stand_up(MotionStatus::FALLEN);
            usleep(10000);
            Walking::GetInstance()->Start();
        }
        usleep(100000);
    }

    Walking::GetInstance()->Stop();
    Walking::GetInstance()->X_MOVE_AMPLITUDE = 0;
    Walking::GetInstance()->Y_MOVE_AMPLITUDE = 0;
    Walking::GetInstance()->A_MOVE_AMPLITUDE = 0;
    usleep(1000000);
    MotionManager::GetInstance()->SetEnable(false);
    MotionManager::GetInstance()->RemoveModule((MotionModule*)Walking::GetInstance());
    linuxMotionTimer.Stop();
    printf(" Done\n");
    cout << " Done\n";
}

void turn(int degrees_to_turn)
{

//    printf("turning...\t");
    cout << "Turning...\n";

    compass.updateData();
    float initial_degrees = floor(compass.getHeadingDegrees());
    float current_degrees = initial_degrees;

    int direction = 1;
    if (degrees_to_turn < 0)
        direction = -1;

    // figure out target degrees
    float target_degrees = current_degrees + degrees_to_turn;
    if (target_degrees < 0) {
        target_degrees = 360 + target_degrees;
    } else if (target_degrees > 360) {
        target_degrees = target_degrees - 360;
    }

    cout << "Current heading: " << current_degrees << " target: " << target_degrees << ".\n";
//    printf("Current & Target heading: %d, %d\n", current_degrees, target_degrees);

    Walking::GetInstance()->LoadINISettings(ini);
    linuxMotionTimer.Start();
    MotionManager::GetInstance()->AddModule((MotionModule*)Walking::GetInstance());
    MotionManager::GetInstance()->ResetGyroCalibration();
    Walking::GetInstance()->X_OFFSET = -4;
    Walking::GetInstance()->Y_OFFSET += 5;
    Walking::GetInstance()->A_MOVE_AMPLITUDE = 23 * direction;
    Walking::GetInstance()->X_MOVE_AMPLITUDE = 0;
    Walking::GetInstance()->Start();

    while ((current_degrees < (target_degrees - 2)) || (current_degrees > (target_degrees + 2)))
    {
        // update current degrees
        compass.updateData();
        current_degrees = floor(compass.getHeadingDegrees());
//        printf("Current heading: %d\n", current_degrees);
        cout << "Current heading: " << current_degrees << "\n";

        // check if we've fallen
        if (MotionStatus::FALLEN != STANDUP)
        {
            Walking::GetInstance()->Stop();
            cout << "Robot fallen " << MotionStatus::FALLEN << ".\n";
            usleep(10000);
            stand_up(MotionStatus::FALLEN);
            usleep(10000);
            Walking::GetInstance()->Start();
        }
    }

    Walking::GetInstance()->Stop();
    Walking::GetInstance()->X_MOVE_AMPLITUDE = 0;
    Walking::GetInstance()->Y_MOVE_AMPLITUDE = 0;
    Walking::GetInstance()->A_MOVE_AMPLITUDE = 0;
    usleep(1000000);
    MotionManager::GetInstance()->SetEnable(false);
    MotionManager::GetInstance()->RemoveModule((MotionModule*)Walking::GetInstance());
    linuxMotionTimer.Stop();
    printf(" Done\n");
    cout << "Done turning\n";

}

void turnCompass(char *compass_direction[]) {
/*
    printf("turning...\t");

    compass.updateData();
    float initial_degrees = floor(compass.getHeadingDegrees());
    float current_degrees = initial_degrees;

    float degrees_to_turn = 0;

    int direction = 1;
    if (degrees_to_turn < 0)
        direction = -1;

    // figure out target degrees
    float target_degrees = current_degrees + degrees_to_turn;
    if (target_degrees < 0) {
        target_degrees = 360 + target_degrees;
    } else if (target_degrees > 360) {
        target_degrees = target_degrees - 360;
    }

    Walking::GetInstance()->LoadINISettings(ini);
    linuxMotionTimer.Start();
    MotionManager::GetInstance()->AddModule((MotionModule*)Walking::GetInstance());
    MotionManager::GetInstance()->ResetGyroCalibration();
    Walking::GetInstance()->X_OFFSET = -4;
    Walking::GetInstance()->Y_OFFSET += 5;
    Walking::GetInstance()->A_MOVE_AMPLITUDE = 23 * direction;
    Walking::GetInstance()->X_MOVE_AMPLITUDE = 0;
    Walking::GetInstance()->Start();

    while ((current_degrees < (target_degrees - 2)) && (current_degrees > (target_degrees + 2))) {
        compass.updateData();
        current_degrees = floor(compass.getHeadingDegrees());
    }

    Walking::GetInstance()->Stop();
    Walking::GetInstance()->X_MOVE_AMPLITUDE = 0;
    Walking::GetInstance()->Y_MOVE_AMPLITUDE = 0;
    Walking::GetInstance()->A_MOVE_AMPLITUDE = 0;
    usleep(1000000);
    MotionManager::GetInstance()->SetEnable(false);
    MotionManager::GetInstance()->RemoveModule((MotionModule*)Walking::GetInstance());
    linuxMotionTimer.Stop();
    printf(" Done\n");
*/

}

void change_current_dir()
{
    char exepath[1024] = {0};
    if (readlink("/proc/self/exe", exepath, sizeof(exepath)) != -1)
        chdir(dirname(exepath));
}

void sighandler(int sig)
{
    struct termios term;
    tcgetattr( STDIN_FILENO, &term );
    term.c_lflag |= ICANON | ECHO;
    tcsetattr( STDIN_FILENO, TCSANOW, &term );

    exit(0);
}



int main(int argc, char *argv[])
{
    signal(SIGABRT, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGQUIT, &sighandler);
    signal(SIGINT,  &sighandler);

    ini = new minIni(INI_FILE_PATH);
    change_current_dir();

    if(Action::GetInstance()->LoadFile(MOTION_FILE_PATH)) 
		printf("Motions Loaded\n");
    
	//mjpg_streamer* streamer = new mjpg_streamer(0, 0);
    //httpd::ini = ini;

    //////////////////// Framework Initialize ////////////////////////////
    if (MotionManager::GetInstance()->Initialize(&arbotixpro) == false)
	{
		printf("Fail to initialize Motion Manager!\n");
		return 0;
	}


    bool on = TRUE;
	int  input1;
    int  input2;
    int  sec = 0;
    int  sign;
    int  prev_page = 0;
	
	ifstream script;

	compass.initializeHMC5883L();

    while(on){
		
		system("clear"); // clear screen
		cout << "******************** Walking and Motion Manager ********************\n";
		
		// Check to see if there is a specified file that user wants
		// to use. Default to script.txt
		if(argc > 1)
		{
			cout << "Using text script " << argv[1] << ".\n\n";
			script.open(argv[1]);
		}
		else
		{
			cout << "No text file specified. Playing default file ""script.txt"".\n\n";
			script.open("script.txt");
		}
		
		// If file opened successfully, start playing script
		if (script.is_open())
		{
			while(!script.eof())
			{
				script >> input1;
				script >> input2;
			
				linuxMotionTimer.Initialize(MotionManager::GetInstance());
				cout << "\nCommand is : " << input1 << "   |   ";
				cout << "Param   is : "   << input2 << endl;
				cout << "    Status: ";
				
				switch(input1){
				case 1: // Walking
					if(prev_page != 8)
					{
						prev_page = 8;
						motion(8);
					}
					
					if(input2 < 0)
						sign = -1;
					else
						sign = 1;
						sec = sign * input2;
						walk(sign, sec);
						break;

				case 2: // Turning
					if(prev_page != 8)
					{
						prev_page = 8;
						motion(8);
					}

					turn(input2);
					break;

				case 3: // Play RME page
					prev_page = input2;
					motion(input2);
					break;
					
				case 4: // Sleep
					printf("Sleeping...\t");
					usleep(input2*1000);
					printf("Done.\n");
					break;	
				
				case 5: // Play MP3
					printf("MP3 Function not developed yet.\n");
					break;
					
				case 6: // Send Midi
					printf("Midi Function not developed yet.\n");
					
				case 7: // End of Script, off
					printf("End of Script.\n");
					script.close();
					break;
					
				default:
					perror("invalid input\n");
					usleep(1500000);
				}
			}
			
			printf("\nClosing Walking and Motion Manager.\n\n");
			exit(0);
		}
		else
		{
			printf("Failed to open text file.\nClosing Walking and Motion Manager.\n\n");
			exit(0);
		}
	}
}


