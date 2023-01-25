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
		virtual void RemoveObject(void* object) = 0;
		void TogglePhysicsState(bool state) { simulate = state; };

		inline bool GetSimulationState() { return simulate; };
	protected:
		bool simulate = false;
	};
};
