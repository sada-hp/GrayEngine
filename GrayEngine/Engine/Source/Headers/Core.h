#pragma once

#ifdef DllExport
	#define DllExport __declspec(dllexport)
#else
	#define DllExport __declspec(dllimport)
#endif
