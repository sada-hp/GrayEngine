#pragma once
#include <btBulletDynamicsCommon.h>

namespace GrEngine
{
	class Physics
	{
	public:
		Physics()
		{
			
		}

		~Physics() {};

		virtual void SimulateStep() = 0;
		virtual void AddSimulationObject(void* object) = 0;
		virtual void AddPhysicsObject(void* object) = 0;
		virtual void RemoveObject(void* object) = 0;
		virtual void RemovePhysicsObject(void* object) = 0;
		virtual void CleanUp() = 0;
		virtual void TogglePhysicsState(bool state) = 0;

		inline bool GetSimulationState() { return simulate; };
	protected:
		bool simulate = false;
		std::vector<void*> objects;
	};
};
