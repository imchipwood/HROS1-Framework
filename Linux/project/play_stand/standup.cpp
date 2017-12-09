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

void ClearCmd()
{
	PrintCmd("");
}

void PrintCmd(const char *message)
{
	int len = strlen(message);

	printf( "] %s", message);
	for (int i = 0; i < (SCREEN_COL - (len + 2)); i++)
		printf(" ");
}

void PlayCmd(ArbotixPro *arbotixpro, int pageNum)
{
	int value, oldIndex = 0;
	Action::PAGE page;

    // Check if we can load the page
	oldIndex = indexPage;
	if (pageNum != indexPage) {
        memcpy(&page, &Page, sizeof(Action::PAGE));
        indexPage = pageNum;
        if (Action::GetInstance()->LoadPage(indexPage, &Page) != true) {
            memcpy(&Page, &page, sizeof(Action::PAGE));
            indexPage = oldIndex;
            return;
        }
    }

    // Check for invalid joint values
	for (int i = 0; i < Page.header.stepnum; i++) {
        for (int id = JointData::ID_MIN; id <= JointData::ID_MAX; id++) {
            if (Page.step[i].position[id] & Action::INVALID_BIT_MASK) {
                PrintCmd("Exist invalid joint value");
                //return;
            }
        }
    }

    // Initialie the joints?
	for (int id = JointData::ID_MIN; id <= JointData::ID_MAX; id++) {
        if (arbotixpro->ReadByte(id, AXDXL::P_TORQUE_ENABLE, &value, 0) == ArbotixPro::SUCCESS) {
            if (value == 0) {
                if (arbotixpro->ReadWord(id, AXDXL::P_PRESENT_POSITION_L, &value, 0) == ArbotixPro::SUCCESS)
                    MotionStatus::m_CurrentJoints.SetValue(id, value);
            } else {
                if (arbotixpro->ReadWord(id, AXDXL::P_GOAL_POSITION_L, &value, 0) == ArbotixPro::SUCCESS)
                    MotionStatus::m_CurrentJoints.SetValue(id, value);
            }
        }
    }

    // Run the motions
	//MotionManager::GetInstance()->StartThread();
	linuxMotionTimer.Start();
	Action::GetInstance()->m_Joint.SetEnableBody(true, true);
	MotionManager::GetInstance()->SetEnable(true);
	if (Action::GetInstance()->Start(pageNum, &Page) == false) {
        PrintCmd("Failed to play this page!\n");
        MotionManager::GetInstance()->SetEnable(false);
        linuxMotionTimer.Stop();
        return;
    }
	//set_stdin();

    // Wait for motions to finish
	while (Action::GetInstance()->IsRunning() == true) {
        usleep(8000);
    }
/*
	while (1) {
        if (Action::GetInstance()->IsRunning() == false)
            break;
        usleep(8000);
    }
*/

    // Disable the motions
	MotionManager::GetInstance()->SetEnable(false);
	linuxMotionTimer.Stop();
	printf("%s ", Page.header.name);
	printf("        Done.\n");

    // Move back to old index if we didn't land there already
	usleep(10000);
	if (oldIndex != indexPage) {
        memcpy(&Page, &page, sizeof(Action::PAGE));
        indexPage = oldIndex;
    }
}