#pragma once

#ifndef _STANDUP_H_
#define _STANDUP_H_

#include "LinuxDARwIn.h"

#define PROGRAM_VERSION "v1.00"

#define DOWN_FACE 1
#define DOWN_BACK -1
#define UPRIGHT 0

void PlayCmd(Robot::ArbotixPro * arbotixpro, int pageNum);



void stand_up(int direction);
{


}

int laying_down(void)'' {

    // TODO: Check if we're laying down


    return UPRIGHT;
}

#endif
