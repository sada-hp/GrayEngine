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
	Custom
};

typedef void (*EventCallbackFun)(std::vector<double> para);

class _declspec(dllexport) EventListener //event observer pattern
{
protected:
	static EventListener* _instance;
	bool bAllowEvents = true;
	bool bAllowCustomEvents = true;

	struct EventBase
	{
		EventType type;
		const char* name = "";
		std::vector<double> para;
	};

	EventListener()
	{
	}

	~EventListener()
	{
	}

	void notify(const EventBase& event);
public:
	void operator=(const EventListener&) = delete;

	static EventListener* GetListener();

	void registerEvent(const EventType& event, EventCallbackFun event_function)
	{
		observers_engine[event].push_back(std::forward<EventCallbackFun>(event_function));
	}
	void registerEvent(const char* event_name, EventCallbackFun event_function)
	{
		observers_custom[event_name].push_back(std::forward<EventCallbackFun>(event_function));
	}

	void pushEvent(const EventType& event, const std::vector<double> para);
	void pushEvent(const char* event, const std::vector<double> para);

	void blockEvents(bool engineEventsEnabled = false, bool customEventsEnabled = false);
	bool pollEngineEvents();
private:
	std::map<EventType, std::vector<EventCallbackFun>> observers_engine;
	std::map<const char*, std::vector<EventCallbackFun>> observers_custom;
	std::queue<EventBase> EventQueue;
};

namespace GrEngine
{
//Event calls shortcut macros

#define MouseClickEvent(lambda) EventListener::GetListener()->registerEvent(EventType::MouseClick, lambda)
#define KeyPressEvent(lambda) EventListener::GetListener()->registerEvent(EventType::KeyPress, lambda)
#define WindowResizeEvent(lambda) EventListener::GetListener()->registerEvent(EventType::WindowResize, lambda)
#define MouseScrollEvent(lambda) EventListener::GetListener()->registerEvent(EventType::Scroll, lambda)
#define MouseMoveEvent(lambda) EventListener::GetListener()->registerEvent(EventType::MouseMove, lambda)
#define WindowClosedEvent(lambda) EventListener::GetListener()->registerEvent(EventType::WindowClosed, lambda)
#define CustomEvent(name, lambda) EventListener::GetListener()->registerEvent(name, lambda)
#define CallEvent(name, para) EventListener::GetListener()->pushEvent(name, para)

//Event calls shortcut macros
}