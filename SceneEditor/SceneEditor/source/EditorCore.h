#pragma once

#ifdef ExportDll
	#define ExportDll __declspec(dllexport)
#else
	#define ExportDll __declspec(dllimport)
#endif