#pragma once

#ifdef DllExport
	#define DllExport __declspec(dllexport)
#else
	#define DllExport __declspec(dllimport)
#endif
#include "EventListener.h"

namespace GrEngine
{
	class DllExport Globals
	{
	public:
		static double delta_time;
	};
}