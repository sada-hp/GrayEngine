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

	void notify(const EventBase& event, bool enabled = true);
public:
	void operator=(const EventListener&) = delete;

	static EventListener* GetListener();

	void pushEvent(const EventType& event, EventCallbackFun event_function)
	{
		observers_engine[event].push_back(std::forward<EventCallbackFun>(event_function));
	}
	void pushEvent(const char* event_name, EventCallbackFun event_function)
	{
		observers_custom[event_name].push_back(std::forward<EventCallbackFun>(event_function));
	}

	void blockEvents(bool engineEventsEnabled = false, bool customEventsEnabled = false);
	bool pollEngineEvents();

	void registerEvent(const EventType& event, const std::vector<double> para);
	void registerEvent(const char* event, const std::vector<double> para);
private:
	std::map<EventType, std::vector<EventCallbackFun>> observers_engine;
	std::map<const char*, std::vector<EventCallbackFun>> observers_custom;
	std::queue<EventBase> EventQueue;
	uint32_t resizeEventsCount = 0;
};

namespace GrEngine
{
//Event calls shortcut macros

#define MouseClickEvent(lambda) EventListener::GetListener()->pushEvent(EventType::MouseClick, lambda)
#define KeyPressEvent(lambda) EventListener::GetListener()->pushEvent(EventType::KeyPress, lambda)
#define WindowResizeEvent(lambda) EventListener::GetListener()->pushEvent(EventType::WindowResize, lambda)
#define MouseScrollEvent(lambda) EventListener::GetListener()->pushEvent(EventType::Scroll, lambda)
#define MouseMoveEvent(lambda) EventListener::GetListener()->pushEvent(EventType::MouseMove, lambda)
#define WindowClosedEvent(lambda) EventListener::GetListener()->pushEvent(EventType::WindowClosed, lambda)
#define CustomEvent(name, lambda) EventListener::GetListener()->pushEvent(name, lambda)
#define CallEvent(name, para) EventListener::GetListener()->registerEvent(name, para)

//Event calls shortcut macros
}