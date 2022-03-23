#pragma once
#include <pch.h>

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
	Custom
};

typedef void (*EventCallbackFun)(std::vector<double>);


class _declspec(dllexport) EventListener //event observer pattern
{

protected:
	static std::unique_ptr<EventListener> _instance;
	bool bAllowEvents = true;
	bool bAllowCustomEvents = true;

	struct EventBase
	{
		EventType type;
		const char* name = "";
		std::vector<double> para;
	};

	static void notify(const EventBase& event, bool enabled = true);

	static EventListener* GetListener();

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
		GetListener()->observers_engine[event].push_back(std::forward<EventCallbackFun>(event_function));
	}
	static void pushEvent(const char* event_name, EventCallbackFun event_function)
	{
		GetListener()->observers_custom[event_name].push_back(std::forward<EventCallbackFun>(event_function));
	}

	static void blockEvents(bool engineEventsEnabled = false, bool customEventsEnabled = false);
	static bool pollEngineEvents();

	static void registerEvent(const EventType& event, const std::vector<double> para);
	static void registerEvent(const char* event, const std::vector<double> para);
private:
	std::map<EventType, std::vector<EventCallbackFun>> observers_engine;
	std::map<const char*, std::vector<EventCallbackFun>> observers_custom;
	std::queue<EventBase> EventQueue;
	uint32_t resizeEventsCount = 0;
};