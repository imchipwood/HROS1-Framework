#pragma once

#ifndef _STANDUP_H_
#define _STANDUP_H_

#include "LinuxDARwIn.h"

#define PROGRAM_VERSION "v1.00"
#define SCREEN_COL      35

int indexPage = 1;
Action::PAGE Page;
//Action::STEP Step;

// pages for standup motions
#define STANDUP_FACE 10
#define STANDUP_BACK 20

// Printing commands
void ClearCmd();
void PrintCmd(const char *message);

// Play RME commands
void PlayCmd(Robot::ArbotixPro * arbotixpro, int pageNum);

#endif
