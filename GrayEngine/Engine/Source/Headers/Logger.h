#pragma once
#include <pch.h>
#include "Events/EventListener.h"

static _declspec(dllexport) enum class OutputColor
{
	Gray = 7,
	Blue = 3,
	Green = 2,
	Red = 4,
	Yellow = 6
};

static _declspec(dllexport) enum class OutputType
{
	Log,
	Error,
	Warning
};

static _declspec(dllexport) class Logger
{
public:

	template<typename ... Args>
	static void Out(const char* message, OutputColor color, OutputType type, const Args&... values)
	{
		auto time_local = GetTime();
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), static_cast<WORD>(color));

		char time_buf[256];
		const char* _time_format = "[%02d-%02d-%d][%02d:%02d:%02d] %s: ";
		const char* log_type = GetTypeBasedString(type);
		std::snprintf(time_buf, sizeof(time_buf), _time_format, time_local.tm_mday, time_local.tm_mon, time_local.tm_year, time_local.tm_hour, time_local.tm_min, time_local.tm_sec, log_type);

		printf(time_buf);
		printf(message, values...);
		printf("\n");

		char _buf[1024];
		std::snprintf(_buf, sizeof(_buf), message, values...);

		std::vector<double> para{};
		std::string msg = time_buf;
		msg += _buf;
		for (char letter : msg)
		{
			para.push_back(letter);
		}
		EventListener::GetListener()->registerEvent(EventType::Log, para);
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
};

