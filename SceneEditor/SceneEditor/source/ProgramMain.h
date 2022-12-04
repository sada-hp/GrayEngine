#pragma once
#include "SceneEditor.h"
#include "source/Application.h"
#include <GrayEngine.h>

namespace SceneEditor
{
    extern "C"
    {
        ExportDll int __stdcall EntryPoint();
    }

    GrEngine::Application* GetApplication();
}