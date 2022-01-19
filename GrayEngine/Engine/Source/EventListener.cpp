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
	switch (event.type)
	{
		case EventType::Custom:
			for (const auto& obs : observers_custom[event.name])
				obs(event.para);
			break;
		default:
			for (const auto& obs : observers_engine[event.type])
				obs(event.para);
			break;
	}
}

void EventListener::pushEvent(const char* name, const std::vector<double> para)
{
	EventBase event;
	event.type = EventType::Custom;
	event.name = name;
	event.para = para;

	EventQueue.push(event);
}

void EventListener::pushEvent(const EventType& type, const std::vector<double> para)
{
	EventBase event;
	event.type = type;
	event.para = para;

	EventQueue.push(event);
}

void EventListener::blockEvents(bool engineEventsEnabled, bool customEventsEnabled)
{
	bAllowEvents = engineEventsEnabled;
	bAllowCustomEvents = customEventsEnabled;
}

bool EventListener::pollEngineEvents()
{
	while (EventQueue.size() > 0)
	{
		if ((bAllowEvents && EventQueue.front().type != EventType::Custom) || (bAllowCustomEvents && EventQueue.front().type == EventType::Custom))
			notify(EventQueue.front());
		EventQueue.pop();
	}

	return true;
}