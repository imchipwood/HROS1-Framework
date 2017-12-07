#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <term.h>
#include <ncurses.h>
#include "standup.h"

using namespace Robot;

extern LinuxMotionTimer linuxMotionTimer;

int indexPage = 1;
Action::PAGE Page;
//Action::STEP Step;

// pages for standup motions
#define STANDUP_FACE 10
#define STANDUP_BACK 20

void stand_up(int direction)
{
    if (direction > 0) {
        motion(STANDUP_FACE);
    } else if (direction < 0) {
        motion(STANDUP_BACK);
    }
}



void ClearCmd()
{
	PrintCmd("");
}

void PrintCmd(const char *message)
{
	int len = strlen(message);
	//GoToCursor(0, CMD_ROW);

	printf( "] %s", message);
	for (int i = 0; i < (SCREEN_COL - (len + 2)); i++)
		printf(" ");

	//GoToCursor(len + 2, CMD_ROW);
}

