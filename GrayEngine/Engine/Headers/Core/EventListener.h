#pragma once
#include <pch.h>
#include "Core/Core.h"

enum class EventType
{
	MouseClick,
	MouseMove,
	KeyPress,
	Scroll,
	WindowResize,
	WindowClosed,
	Step,
	Log,
	Custom,
	SelectionChanged,
	FocusChanged
};

typedef void (*EventCallback)(std::vector<double>);
typedef void (*EventCallbackCustom)(std::vector<std::any>);


class DllExport EventListener //event observer pattern
{
protected:
	bool bAllowEvents = true;
	bool bAllowCustomEvents = true;

	struct EventBase
	{
		EventType type;
		std::vector<double> para;
	};

	struct EventBaseCustom
	{
		EventType type;
		const char* name = "";
		std::vector<std::any> para;
	};

	void notify(const EventBase& event)
	{
		if (event.type == EventType::WindowResize && resizeEventsCount > 1 && resizeEventsCount--) return; //Resize event should be notified only about it's latest state

		switch (event.type)
		{
		case EventType::WindowResize:
			for (const auto& callback : observers_engine[event.type])
				callback(event.para);
			resizeEventsCount = 0;
			break;
		default:
			for (const auto& callback : observers_engine[event.type])
				callback(event.para);
			break;
		}
	}

	void notify(const EventBaseCustom& event)
	{
		for (const auto& callback : observers_custom[event.name])
			callback(event.para);
	}

public:
	EventListener()
	{
	}

	~EventListener()
	{
	}

	void subscribe(const EventType& event, EventCallback event_function)
	{
		if (bAllowEvents)
			observers_engine[event].push_back(std::forward<EventCallback>(event_function));
	}
	void subscribe(const char* event_name, EventCallbackCustom event_function)
	{
		if (bAllowCustomEvents)
			observers_custom[event_name].push_back(std::forward<EventCallbackCustom>(event_function));
	}

	void setEventsPermissions(bool engineEventsEnabled, bool customEventsEnabled)
	{
		bAllowEvents = engineEventsEnabled;
		bAllowCustomEvents = customEventsEnabled;
	}

	bool pollEngineEvents()
	{
		if (!bAllowEvents && !bAllowCustomEvents) return false;

		while (EventQueue.size() > 0)
		{
			if (bAllowEvents)
			{
				notify(EventQueue.front());
			}

			EventQueue.pop();
		}

		while (CustomEventQueue.size() > 0)
		{
			if (bAllowCustomEvents)
			{
				notify(CustomEventQueue.front());
			}

			CustomEventQueue.pop();
		}

		clearEventQueue();
		return true;
	}

	void registerEvent(const char* name, std::vector<std::any> para)
	{
		if (!bAllowCustomEvents) return;

		EventBaseCustom event;
		event.type = EventType::Custom;
		event.name = name;
		event.para = para;

		CustomEventQueue.push(event);
	}

	void registerEvent(const EventType& type, std::vector<double> para)
	{
		if (!bAllowEvents) return;

		if (type == EventType::Custom)
			throw std::runtime_error("Use const char* to define custom event!");
		else
			resizeEventsCount += (type == EventType::WindowResize);

		EventBase event;
		event.type = type;
		event.para = para;

		EventQueue.push(event);	
	}

	void clearEventQueue()
	{
		while (EventQueue.size() > 0)
		{
			EventQueue.pop();
		}

		while (CustomEventQueue.size() > 0)
		{
			CustomEventQueue.pop();
		}
	}
private:
	std::map<EventType, std::vector<EventCallback>> observers_engine;
	std::map<const char*, std::vector<EventCallbackCustom>> observers_custom;
	std::queue<EventBase> EventQueue;
	std::queue<EventBaseCustom> CustomEventQueue;
	uint32_t resizeEventsCount = 0;
};