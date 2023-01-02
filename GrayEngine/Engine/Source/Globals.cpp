#include <pch.h>
#include "Core/EventListener.h"
#include "Core/Globals.h"
#include "Virtual/Physics.h"

double GrEngine::Globals::delta_time = 1;
std::unique_ptr<EventListener> EventListener::global_listener = nullptr;
GrEngine::Physics* GrEngine::Physics::context = nullptr;
