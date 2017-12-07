#pragma once

#ifndef _STANDUP_H_
#define _STANDUP_H_

#include "LinuxDARwIn.h"

#define PROGRAM_VERSION "v1.00"

// Printing commands
void ClearCmd();
void PrintCmd(const char *message);

// Play RME commands
void PlayCmd(Robot::ArbotixPro * arbotixpro, int pageNum);

// Function that calls correct motion to stand based on current fallen direction
void stand_up(int direction);

#endif
