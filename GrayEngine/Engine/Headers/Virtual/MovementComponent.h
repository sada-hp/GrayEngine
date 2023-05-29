#pragma once
#include <glm/glm.hpp>

namespace GrEngine
{
	class Entity;

	class MovementComponent
	{
	public:
		MovementComponent(Entity* owner)
		{
			pOwner = owner;
		}

		virtual ~MovementComponent()
		{

		}

		virtual void UpdateController() = 0;
		virtual void ResetVelocity() = 0;
		virtual void MoveObject(glm::vec3 vector) = 0;
		virtual void SlideObjectForDuration(glm::vec3 vector, float dur) = 0;
		virtual void Dispose() = 0;
		virtual bool IsGrounded() = 0;
		virtual void Jump() = 0;

	protected:
		Entity* pOwner;
	};
}