#pragma once
#include <glm/glm.hpp>
#include "Core.h"
#include <glm/gtc/quaternion.hpp>

namespace GrEngine
{
	class Camera
	{
	public:
		Camera() {};
		~Camera() {};

		glm::vec3 cam_pos;
		glm::vec3 cam_rot = { 0.f, 0.f, 0.f};
		glm::vec2 cursor_pos;
		glm::quat cam_orientation;
	};
}