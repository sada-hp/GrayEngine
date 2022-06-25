#pragma once
#include <pch.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Core.h"

namespace GrEngine
{
	struct Texture
	{
		const char* texture_path;
	};

	struct Vertex
	{
		glm::vec4 pos;
		glm::vec4 color;
		glm::vec2 uv;
		uint32_t uv_index;

		bool operator==(const Vertex& other) const
		{
			return pos == other.pos && uv == other.uv;
		}
	};

	struct Mesh
	{
		const char* mesh_path;
	};

	class DllExport DrawableObject
	{
	public:
		DrawableObject() {};
		virtual ~DrawableObject() {};

		void Rotate(float pitch, float yaw, float roll)
		{
			pitch_yaw_roll += glm::vec3{pitch, yaw, roll};
			glm::quat qPitch = glm::angleAxis(glm::radians(pitch_yaw_roll.x), glm::vec3(1, 0, 0));
			glm::quat qYaw = glm::angleAxis(glm::radians(pitch_yaw_roll.y), glm::vec3(0, 1, 0));
			glm::quat qRoll = glm::angleAxis(glm::radians(pitch_yaw_roll.z), glm::vec3(0, 0, 1));
			obj_orientation = glm::normalize(qPitch * qYaw * qRoll);
		}
		void Rotate(glm::vec3 angle)
		{
			pitch_yaw_roll += angle;
			glm::quat qPitch = glm::angleAxis(glm::radians(pitch_yaw_roll.x), glm::vec3(1, 0, 0));
			glm::quat qYaw = glm::angleAxis(glm::radians(pitch_yaw_roll.y), glm::vec3(0, 1, 0));
			glm::quat qRoll = glm::angleAxis(glm::radians(pitch_yaw_roll.z), glm::vec3(0, 0, 1));
			obj_orientation += glm::normalize(qPitch * qYaw * qRoll);
		}
		void SetRotation(float pitch, float yaw, float roll)
		{
			pitch_yaw_roll = glm::vec3{ pitch, yaw, roll };
			glm::quat qPitch = glm::angleAxis(glm::radians(pitch_yaw_roll.x), glm::vec3(1, 0, 0));
			glm::quat qYaw = glm::angleAxis(glm::radians(pitch_yaw_roll.y), glm::vec3(0, 1, 0));
			glm::quat qRoll = glm::angleAxis(glm::radians(pitch_yaw_roll.z), glm::vec3(0, 0, 1));
			obj_orientation = glm::normalize(qPitch * qYaw * qRoll);
		}
		void SetRotation(glm::vec3 angle)
		{
			pitch_yaw_roll = angle;
			glm::quat qPitch = glm::angleAxis(glm::radians(pitch_yaw_roll.x), glm::vec3(1, 0, 0));
			glm::quat qYaw = glm::angleAxis(glm::radians(pitch_yaw_roll.y), glm::vec3(0, 1, 0));
			glm::quat qRoll = glm::angleAxis(glm::radians(pitch_yaw_roll.z), glm::vec3(0, 0, 1));
			obj_orientation = glm::normalize(qPitch * qYaw * qRoll);
		}
		glm::vec3& getObjectBounds()
		{
			return bound;
		}
		void setObjectBounds(glm::vec3 new_bounds)
		{
			bound = new_bounds;
		}
	protected:
		glm::quat obj_orientation;
		glm::vec3 pitch_yaw_roll;
		glm::vec3 bound;
	};
}
