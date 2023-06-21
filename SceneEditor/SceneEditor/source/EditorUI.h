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

	typedef void(*UpdateMaterialsFunc)(char*, int);
	UpdateMaterialsFunc UpdateMaterials;

	typedef void(*UpdateEntityFunc)(int, char*);
	UpdateEntityFunc UpdateEntity;

	typedef void(*SendInfoChunkCall)(int, char*, char*);
	SendInfoChunkCall SendInfoChunk;

	typedef void(*RemoveEntityFunc)(int);
	RemoveEntityFunc RemoveEntity;

	typedef void(*SetInputModeFunc)(UINT, int);
	SetInputModeFunc SetInputMode;

	typedef void(*ShowContextFunc)();
	ShowContextFunc ShowContextMenu;

	EditorUI()
	{
		dotNetGUILibrary = LoadLibraryA("EditorUI.dll");
		if (dotNetGUILibrary == nullptr)
		{
			Logger::Out("Failed to load UI", OutputType::Error);
		}
		CreateUserInterface = (CreateUserInterfaceFunc)GetProcAddress(dotNetGUILibrary, "CreateUserInterface");
		DisplayUserInterface = (DisplayUserInterfaceFunc)GetProcAddress(dotNetGUILibrary, "DisplayUserInterface");
		DestroyUserInterface = (DestroyUserInterfaceFunc)GetProcAddress(dotNetGUILibrary, "DestroyUserInterface");
		ParentRenderer = (GetRendererViewportFunc)GetProcAddress(dotNetGUILibrary, "ParentRenderer");
		SetViewportPosition = (SetChildPositionFunc)GetProcAddress(dotNetGUILibrary, "UpdateChildPosition");
		UpdateMaterials = (UpdateMaterialsFunc)GetProcAddress(dotNetGUILibrary, "PassMaterialString");
		UpdateEntity = (UpdateEntityFunc)GetProcAddress(dotNetGUILibrary, "UpdateEntity");
		SendInfoChunk = (SendInfoChunkCall)GetProcAddress(dotNetGUILibrary, "RecieveInfoChunk");
		RemoveEntity = (RemoveEntityFunc)GetProcAddress(dotNetGUILibrary, "RemoveEntity");
		SetInputMode = (SetInputModeFunc)GetProcAddress(dotNetGUILibrary, "SetInputMode");
		ShowContextMenu = (ShowContextFunc)GetProcAddress(dotNetGUILibrary, "OpenContextMenuForMainWindow");
	};

	~EditorUI()
	{
	};

	bool InitUI(UINT viewport_index)
	{
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		wpf_hwnd = CreateUserInterface(viewport_index);
		if (wpf_hwnd != nullptr)
		{
			DisplayUserInterface(viewport_index);
			return true;
		}
		return false;
	}

	bool destroyUI(UINT viewport_index)
	{
		if (wpf_hwnd != NULL)
		{
			//CoUninitialize();
			DestroyUserInterface(viewport_index);
			DestroyWindow(wpf_hwnd);
		}

		FreeLibrary(dotNetGUILibrary);
		//CoUninitialize();
		return true;
	}

	void SetViewportHWND(HWND child, UINT viewport_index)
	{
		ShowWindow(child, SW_HIDE); // hide the window
		long style = GetWindowLong(child, GWL_STYLE);
		style &= ~WS_POPUP; // remove popup style
		style &= ~WS_BORDER; // remove border style
		style &= WS_THICKFRAME; // remove border style
		style |= WS_CHILDWINDOW; // add childwindow style
		SetWindowLong(child, GWL_STYLE, style);
		ShowWindow(child, SW_SHOW); // show the window for the new style to come into effect

		ParentRenderer(child, viewport_index);
		glfw_hwnd = child;
	}

	void ShowScene()
	{
		ShowWindow(glfw_hwnd, SW_SHOW);
	}
};