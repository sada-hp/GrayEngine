#include "pch.h"
#include "Headers/Events/EventListener.h"

EventListener* EventListener::_instance = nullptr;

EventListener* EventListener::GetListener() //Singleton
{
	if (_instance == nullptr)
		_instance = new EventListener();
	return EventListener::_instance;
}

void EventListener::notify(const EventBase& event)
{
	for (const auto& obs : observers_[event.type]) obs(event.para);
}

void EventListener::pushEvent(const EventType& type, const std::vector<double> para)
{
	EventBase event;
	event.type = type;
	event.para = para;

	EventQueue.push(event);
}

void EventListener::blockEvents()
{
	bAllowEvents = false;
	EventQueue.empty();
}

void EventListener::allowEvents()
{
	bAllowEvents = true;
}

bool EventListener::pollEngineEvents()
{
	while (bAllowEvents && EventQueue.size() > 0)
	{
		notify(EventQueue.front());
		EventQueue.pop();
	}
	return bAllowEvents;
}