#include <pch.h>
#include "EventListener.h"

std::unique_ptr<EventListener> EventListener::_instance = nullptr;

EventListener* EventListener::GetListener() //Singleton
{
	if (_instance == nullptr)
		_instance = std::make_unique<EventListener>();
	return EventListener::_instance.get();
}

void EventListener::notify(const EventBase& event, bool enabled)
{
	if (!enabled || (event.type == EventType::WindowResize && GetListener()->resizeEventsCount > 1 && GetListener()->resizeEventsCount--)) return; //Resize event should be notified only about it's latest state

	switch (event.type)
	{
		case EventType::Custom:
			for (const auto& callback : GetListener()->observers_custom[event.name])
				callback(event.para);
			break;
		case EventType::WindowResize:
			for (const auto& callback : GetListener()->observers_engine[event.type])
				callback(event.para);
			GetListener()->resizeEventsCount = 0;
			break;
		default:
			for (const auto& callback : GetListener()->observers_engine[event.type])
				callback(event.para);
			break;
	}
}

void EventListener::registerEvent(const char* name, const std::vector<double> para)
{
	EventBase event;
	event.type = EventType::Custom;
	event.name = name;
	event.para = para;

	GetListener()->EventQueue.push(event);
}

void EventListener::registerEvent(const EventType& type, const std::vector<double> para)
{
	if (type == EventType::Custom)
		throw std::runtime_error("Use const char* to define custom event!");
	else
		GetListener()->resizeEventsCount += (type == EventType::WindowResize);

	EventBase event;
	event.type = type;
	event.para = para;

	GetListener()->EventQueue.push(event);
}

void EventListener::blockEvents(bool engineEventsEnabled, bool customEventsEnabled)
{
	GetListener()->bAllowEvents = engineEventsEnabled;
	GetListener()->bAllowCustomEvents = customEventsEnabled;
}

bool EventListener::pollEngineEvents()
{
	if (!GetListener()->bAllowEvents && !GetListener()->bAllowCustomEvents) return false;

	while (GetListener()->EventQueue.size() > 0)
	{
		if ((GetListener()->bAllowEvents && GetListener()->EventQueue.front().type != EventType::Custom) || (GetListener()->bAllowCustomEvents && GetListener()->EventQueue.front().type == EventType::Custom))
		{
			notify(GetListener()->EventQueue.front());
		}

		GetListener()->EventQueue.pop();
	}

	return true;
}