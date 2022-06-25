#include <pch.h>
#include "EventListener.h"
#include "DrawableObject.h"

std::unique_ptr<EventListener> EventListener::_instance = nullptr;

EventListener* EventListener::GetListener() //Singleton
{
	if (_instance == nullptr)
		_instance = std::make_unique<EventListener>();
	return EventListener::_instance.get();
}