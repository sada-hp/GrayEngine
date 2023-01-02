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
		inline static Physics* GetContext() { return context; };

		static void SetContext(Physics* new_context)
		{
			if (context != nullptr)
			{
				return;
			}

			context = new_context;
		}

	private:
		static Physics* context;
	protected:
		bool simulate = false;
	};
};
