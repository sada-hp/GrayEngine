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
	SelectionChanged
};

typedef void (*EventCallbackFun)(std::vector<std::any>);


class DllExport EventListener //event observer pattern
{
protected:
	static std::unique_ptr<EventListener> global_listener;
	bool bAllowEvents = true;
	bool bAllowCustomEvents = true;

	struct EventBase
	{
		EventType type;
		const char* name = "";
		std::vector<std::any> para;
	};

	static void notify(const EventBase& event)
	{
		if (event.type == EventType::WindowResize && GetListener()->resizeEventsCount > 1 && GetListener()->resizeEventsCount--) return; //Resize event should be notified only about it's latest state

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

	inline static EventListener* GetListener() //Singleton
	{
		if (global_listener == nullptr)
			global_listener = std::make_unique<EventListener>();
		return EventListener::global_listener.get();
	}

public:
	EventListener()
	{
	}

	~EventListener()
	{
	}

	void operator=(const EventListener&) = delete;

	static void pushEvent(const EventType& event, EventCallbackFun event_function)
	{
		if (GetListener()->bAllowEvents)
			GetListener()->observers_engine[event].push_back(std::forward<EventCallbackFun>(event_function));
	}
	static void pushEvent(const char* event_name, EventCallbackFun event_function)
	{
		if (GetListener()->bAllowCustomEvents)
			GetListener()->observers_custom[event_name].push_back(std::forward<EventCallbackFun>(event_function));
	}

	static void setEventsPermissions(bool engineEventsEnabled, bool customEventsEnabled)
	{
		GetListener()->bAllowEvents = engineEventsEnabled;
		GetListener()->bAllowCustomEvents = customEventsEnabled;
	}

	static bool pollEngineEvents()
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

		clearEventQueue();
		return true;
	}

	static void registerEvent(const char* name, std::vector<std::any> para)
	{
		if (!GetListener()->bAllowCustomEvents) return;

		EventBase event;
		event.type = EventType::Custom;
		event.name = name;
		event.para = para;

		GetListener()->EventQueue.push(event);
	}

	static void registerEvent(const EventType& type, std::vector<std::any> para)
	{
		if (!GetListener()->bAllowEvents) return;

		if (type == EventType::Custom)
			throw std::runtime_error("Use const char* to define custom event!");
		else
			GetListener()->resizeEventsCount += (type == EventType::WindowResize);

		EventBase event;
		event.type = type;
		event.para = para;

		GetListener()->EventQueue.push(event);	
	}

	static void clearEventQueue()
	{
		GetListener()->EventQueue.empty();
	}
private:
	std::map<EventType, std::vector<EventCallbackFun>> observers_engine;
	std::map<const char*, std::vector<EventCallbackFun>> observers_custom;
	std::queue<EventBase> EventQueue;
	uint32_t resizeEventsCount = 0;
};