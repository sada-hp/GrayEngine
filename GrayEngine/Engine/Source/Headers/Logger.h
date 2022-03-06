#pragma once
#include <pch.h>

static _declspec(dllexport) enum class OutputColor
{
	Gray = 7,
	Blue = 3,
	Green = 2,
	Red = 4,
	Yellow = 6
};

static _declspec(dllexport) class Logger
{
public:
	template<typename ... Args>
	static void Out(const char* message, OutputColor color, const Args&... values)
	{
		auto time_local = GetTime();
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), static_cast<WORD>(color));
		printf("[%02d-%02d-%d][%02d:%02d:%02d] ", time_local.tm_mday, time_local.tm_mon, time_local.tm_year, time_local.tm_hour, time_local.tm_min, time_local.tm_sec);
		printf(message, values...);
		printf("\n");
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
};