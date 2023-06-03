#pragma once
#include <pch.h>
#include "Core/Core.h"
#include "EventListener.h"

static enum class OutputType
{
	Log,
	Error,
	Warning
};

static enum class MessageMode
{
	Allow,
	Block,
	GetValue
};

class DllExport Logger
{
public:

	template<typename ... Args>
	static void Out(const char* message, OutputType type, const Args&... values)
	{
		if (!AllowMessages(MessageMode::GetValue)) return;

		auto time_local = GetTime();
		char time_buf[256];
		char _buf[1024];
		const char* log_type = GetTypeBasedString(type);

		std::snprintf(_buf, sizeof(_buf), message, values...);
		std::snprintf(time_buf, sizeof(time_buf), "[%02d-%02d-%d][%02d:%02d:%02d] %s: ", time_local.tm_mday, time_local.tm_mon, time_local.tm_year, time_local.tm_hour, time_local.tm_min, time_local.tm_sec, log_type);

		std::string msg = std::string(time_buf) + std::string(_buf);

		printf("%s\n", msg.c_str());

		if (eListener != nullptr)
		{
			std::vector<double> para{};
			for (char letter : msg)
			{
				para.push_back(letter);
			}

			eListener->registerEvent(EventType::Log, para);
		}
	}

	static void ShowConsole(bool show)
	{
		ShowWindow(GetConsoleWindow(), show);
	}

	static bool AllowMessages(MessageMode logMode)
	{
		static bool isAllowed;
		switch (logMode)
		{
			case (MessageMode::Allow):
				isAllowed = true;
				return isAllowed;
			case (MessageMode::Block):
				isAllowed = false;
				return isAllowed;
			case (MessageMode::GetValue):
				return isAllowed;
			default:
				isAllowed = true;
				return isAllowed;
		}
	}

	static void JoinEventListener(EventListener* listener)
	{
		eListener = listener;
	}
private:

	static struct tm GetTime()
	{
		struct tm newtime;
		__time32_t aclock;

		_time32(&aclock);
		_localtime32_s(&newtime, &aclock);

		newtime.tm_mon++;
		newtime.tm_year+=1900;

		return newtime;
	}

	static const char* GetTypeBasedString(OutputType type)
	{
		switch (type)
		{
		case OutputType::Log:
			return "LOG";
		case OutputType::Error:
			return "ERROR";
		case OutputType::Warning:
			return "WARNING";
		default:
			return "LOG";
		}
	}

	static EventListener* eListener;
};

