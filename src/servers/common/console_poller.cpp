#include "console_poller.h"
#include <def/server.h>
#include <utils/platform.h>
#ifndef PLATFORM_WIN32
#include <poll.h>
#include <stdio.h>
#else
#include <Windows.h>
#endif

ConsolePoller::ConsolePoller()
	: mIsRunning(false)
{
}

ConsolePoller::~ConsolePoller()
{
	mIsRunning = false;
}

bool ConsolePoller::Start(const std::function<void(const char*)>& handler)
{
	if (handler == nullptr) return false;
	if (mIsRunning) return false;

	mHandler = handler;

	mThread.swap(std::thread([&]()
	{
		size_t i = 0;
		size_t len;
		char cmd[300];
#ifndef PLATFORM_WIN32
		struct pollfd input;

		input.fd = 0;
		input.events = POLLIN | POLLPRI;
		input.revents = 0;
#endif

		mIsRunning = true;
		while (mIsRunning)
		{
#ifdef PLATFORM_WIN32
			// Read in single line from "stdin"
			memset(cmd, 0, sizeof(cmd));
			if (fgets(cmd, 300, stdin) == NULL)
				continue;

			if (!mIsRunning)
				break;
#else
			int ret = poll(&input, 1, 1000);
			if (ret < 0)
			{
				break;
			}
			else if (ret == 0)
			{
				if (mIsRunning)	// timeout
					continue;
				else
					break;
			}

			ret = read(0, cmd, sizeof(cmd));
			if (ret <= 0)
			{
				break;
			}
#endif

			len = strlen(cmd);
			for (i = 0; i < len; ++i)
			{
				if (cmd[i] == '\n' || cmd[i] == '\r')
					cmd[i] = '\0';
			}

			mHandler(cmd);
		}

		return false;
	}));
	return true;
}

void ConsolePoller::Stop()
{
	mIsRunning = false;
#ifdef PLATFORM_WIN32
	DWORD dwTmp;
	INPUT_RECORD ir[2];

	ir[0].EventType = KEY_EVENT;
	ir[0].Event.KeyEvent.bKeyDown          = TRUE;
	ir[0].Event.KeyEvent.dwControlKeyState = 288;
	ir[0].Event.KeyEvent.uChar.AsciiChar   = 13;
	ir[0].Event.KeyEvent.wRepeatCount      = 1;
	ir[0].Event.KeyEvent.wVirtualKeyCode   = 13;
	ir[0].Event.KeyEvent.wVirtualScanCode  = 28;

	ir[1].EventType = KEY_EVENT;
	ir[1].Event.KeyEvent.bKeyDown          = FALSE;
	ir[1].Event.KeyEvent.dwControlKeyState = 288;
	ir[1].Event.KeyEvent.uChar.AsciiChar   = 13;
	ir[1].Event.KeyEvent.wRepeatCount      = 1;
	ir[1].Event.KeyEvent.wVirtualKeyCode   = 13;
	ir[1].Event.KeyEvent.wVirtualScanCode  = 28;

	WriteConsoleInput(GetStdHandle(STD_INPUT_HANDLE), ir, 2, &dwTmp);
#endif
	if (std::this_thread::get_id() != mThread.get_id())
	{
		printf("waiting for console thread to quit\n");
		mThread.join();
	}
}
