#pragma once

#define _USE_MATH_DEFINES

#ifdef DllExport
	#define DllExport __declspec(dllexport)
#else
	#define DllExport __declspec(dllimport)
#endif