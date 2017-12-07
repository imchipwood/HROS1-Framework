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

Action::PAGE Page;
//Action::STEP Step;

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

void PlayCmd(ArbotixPro *arbotixpro, int pageNum)
{
	int value, oldIndex = 0;
	Action::PAGE page;

	oldIndex = indexPage;
	if (pageNum != indexPage)
		{
			memcpy(&page, &Page, sizeof(Action::PAGE));
			indexPage = pageNum;
			if (Action::GetInstance()->LoadPage(indexPage, &Page) != true)
				{
					memcpy(&Page, &page, sizeof(Action::PAGE));
					indexPage = oldIndex;
					return;
				}
		}
	for (int i = 0; i < Page.header.stepnum; i++)
		{
			for (int id = JointData::ID_MIN; id <= JointData::ID_MAX; id++)
				{
					if (Page.step[i].position[id] & Action::INVALID_BIT_MASK)
						{
							PrintCmd("Exist invalid joint value");
							//return;
						}
				}
		}

	for (int id = JointData::ID_MIN; id <= JointData::ID_MAX; id++)
		{
			if (arbotixpro->ReadByte(id, AXDXL::P_TORQUE_ENABLE, &value, 0) == ArbotixPro::SUCCESS)
				{
					if (value == 0)
						{
							if (arbotixpro->ReadWord(id, AXDXL::P_PRESENT_POSITION_L, &value, 0) == ArbotixPro::SUCCESS)
								MotionStatus::m_CurrentJoints.SetValue(id, value);
						}
					else
						{
							if (arbotixpro->ReadWord(id, AXDXL::P_GOAL_POSITION_L, &value, 0) == ArbotixPro::SUCCESS)
								MotionStatus::m_CurrentJoints.SetValue(id, value);
						}
				}
		}

	//MotionManager::GetInstance()->StartThread();
	linuxMotionTimer.Start();
	Action::GetInstance()->m_Joint.SetEnableBody(true, true);
	MotionManager::GetInstance()->SetEnable(true);
	if (Action::GetInstance()->Start(pageNum, &Page) == false)
		{
			PrintCmd("Failed to play this page!\n");
			MotionManager::GetInstance()->SetEnable(false);
			linuxMotionTimer.Stop();
			return;
		}
//	set_stdin();
	while (1)
		{
			if (Action::GetInstance()->IsRunning() == false)
				break;
			usleep(8000);
		}

	MotionManager::GetInstance()->SetEnable(false);
	linuxMotionTimer.Stop();
	printf("%s ", Page.header.name);
	printf("        Done.\n");

	usleep(10000);
	if (oldIndex != indexPage)
		{
			memcpy(&Page, &page, sizeof(Action::PAGE));
			indexPage = oldIndex;
		}
}