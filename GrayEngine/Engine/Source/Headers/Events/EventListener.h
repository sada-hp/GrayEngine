#pragma once
#include <functional>
#include <vector>
#include <map>
#include <utility>
#include <any>
#include <queue>

enum class EventType
{
	MouseClick,
	MouseMove,
	KeyPress,
	Scroll,
	WindowResize,
	Custom //TODO
};

typedef void (*EventCallbackFun)(std::vector<double> para);

class _declspec(dllexport) EventListener //event observer pattern
{
protected:
	static EventListener* _instance;
	bool bAllowEvents = true;

	struct EventBase
	{
		EventType type;
		std::vector<double> para;
	};

	EventListener()
	{
	}

	void notify(const EventBase& event);
public:
	void operator=(const EventListener&) = delete;

	static EventListener* GetListener();

	void registerEvent(const EventType& event, EventCallbackFun event_function)
	{
		observers_[event].push_back(std::forward<EventCallbackFun>(event_function));
	}

	void blockEvents();
	void allowEvents();

	void pushEvent(const EventType& event, const std::vector<double>);

	bool pollEngineEvents();
	void notifyCustomEvent(const char* eventName) {}; //TODO

private:
	std::map<EventType, std::vector<std::function<void(std::vector<double>)>>> observers_;
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
#define CustomEvent(lambda) EventListener::GetListener()->registerEvent(EventType::Custom, lambda)

//Event calls shortcut macros
}