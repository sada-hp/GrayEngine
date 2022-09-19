#include <pch.h>
#include "Engine/Source/Headers/EventListener.h"
#include "Globals.h"

double GrEngine::Globals::delta_time = 1;
std::unique_ptr<EventListener> EventListener::global_listener = nullptr;

