#include <pch.h>
#include "Core.h"

double GrEngine::Globals::delta_time = 1;
std::unique_ptr<EventListener> EventListener::global_listener = nullptr;

