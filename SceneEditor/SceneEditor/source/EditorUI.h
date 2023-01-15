#pragma once
#include "EditorCore.h"
#include <GrayEngine.h>

#define VIEWPORT_EDITOR 0
#define VIEWPORT_MODEL_BROWSER 1

class EditorUI
{
	HMODULE dotNetGUILibrary;

public:
	HWND wpf_hwnd; /// WPF Wrapper Handle
	HWND glfw_hwnd;

	typedef HWND(*CreateUserInterfaceFunc)(UINT);
	CreateUserInterfaceFunc CreateUserInterface;

	typedef void(*GetRendererViewportFunc)(HWND, UINT);
	GetRendererViewportFunc ParentRenderer;

	typedef void(*SetChildPositionFunc)(UINT);
	SetChildPositionFunc SetViewportPosition;

	typedef void(*DisplayUserInterfaceFunc)(UINT);
	DisplayUserInterfaceFunc DisplayUserInterface;

	typedef void(*DestroyUserInterfaceFunc)(UINT);
	DestroyUserInterfaceFunc DestroyUserInterface;

	typedef void(*UpdateLoggerFunc)(char*);
	UpdateLoggerFunc UpdateLogger;

	typedef void(*UpdateFramecounterFunc)(double);
	UpdateFramecounterFunc UpdateFramecounter;

	typedef void(*UpdateMaterialsFunc)(char*, char*, int);
	UpdateMaterialsFunc UpdateMaterials;

	typedef void(*UpdateEntityFunc)(int, char*);
	UpdateEntityFunc UpdateEntity;

	typedef void(*RetrieveInfoFunc)(int, char*, char*, char*);
	RetrieveInfoFunc SendEntityInfo;

	typedef void(*SelectEntityFunc)(int);
	SelectEntityFunc SetSelectedEntity;

	typedef void(*RemoveEntityFunc)(int);
	RemoveEntityFunc RemoveEntity;

	typedef void(*SetInputModeFunc)(UINT, int);
	SetInputModeFunc SetInputMode;

	EditorUI()
	{
		dotNetGUILibrary = LoadLibraryA("EditorUI.dll");
		if (dotNetGUILibrary == nullptr)
		{
			Logger::Out("Failed to load UI", OutputColor::Red, OutputType::Error);
		}
		CreateUserInterface = (CreateUserInterfaceFunc)GetProcAddress(dotNetGUILibrary, "CreateUserInterface");
		DisplayUserInterface = (DisplayUserInterfaceFunc)GetProcAddress(dotNetGUILibrary, "DisplayUserInterface");
		DestroyUserInterface = (DestroyUserInterfaceFunc)GetProcAddress(dotNetGUILibrary, "DestroyUserInterface");
		ParentRenderer = (GetRendererViewportFunc)GetProcAddress(dotNetGUILibrary, "ParentRenderer");
		UpdateLogger = (UpdateLoggerFunc)GetProcAddress(dotNetGUILibrary, "UpdateLogger");
		SetViewportPosition = (SetChildPositionFunc)GetProcAddress(dotNetGUILibrary, "UpdateChildPosition");
		UpdateFramecounter = (UpdateFramecounterFunc)GetProcAddress(dotNetGUILibrary, "UpdateFrameCounter");
		UpdateMaterials = (UpdateMaterialsFunc)GetProcAddress(dotNetGUILibrary, "PassMaterialString");
		UpdateEntity = (UpdateEntityFunc)GetProcAddress(dotNetGUILibrary, "UpdateEntity");
		SendEntityInfo = (RetrieveInfoFunc)GetProcAddress(dotNetGUILibrary, "RetrieveEntityInfo");
		SetSelectedEntity = (SelectEntityFunc)GetProcAddress(dotNetGUILibrary, "SetSelectedEntity");
		RemoveEntity = (RemoveEntityFunc)GetProcAddress(dotNetGUILibrary, "RemoveEntity");
		SetInputMode = (SetInputModeFunc)GetProcAddress(dotNetGUILibrary, "SetInputMode");
	};

	~EditorUI()
	{
		DestroyWindow(wpf_hwnd);
	};

	bool InitUI(UINT viewport_index)
	{
		//CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		wpf_hwnd = CreateUserInterface(viewport_index);
		if (wpf_hwnd != nullptr)
		{
			DisplayUserInterface(viewport_index);
			//SetActiveWindow(wpf_hwnd);
		}

		return true;
	}

	bool destroyUI(UINT viewport_index)
	{
		DestroyUserInterface(viewport_index);
		DestroyWindow(wpf_hwnd);

		return true;
	}

	void SetViewportHWND(HWND child, UINT viewport_index)
	{
		long style = GetWindowLong(child, GWL_STYLE);
		style &= ~WS_POPUP; // remove popup style
		style &= ~WS_BORDER; // remove border style
		style &= WS_THICKFRAME; // remove border style
		style |= WS_CHILDWINDOW; // add childwindow style
		SetWindowLong(child, GWL_STYLE, style);

		ParentRenderer(child, viewport_index);
		glfw_hwnd = child;
	}

	void ShowScene()
	{
		ShowWindow(glfw_hwnd, SW_SHOW);
	}
};