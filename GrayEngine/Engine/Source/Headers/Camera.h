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

		void Rotate(float pitch, float yaw, float roll)
		{
			pitch_yaw_roll += glm::vec3{ pitch, yaw, roll };
			checkBorders();
			glm::quat qPitch = glm::angleAxis(glm::radians(pitch_yaw_roll.y), glm::vec3(1, 0, 0));
			glm::quat qYaw = glm::angleAxis(glm::radians(pitch_yaw_roll.x), glm::vec3(0, 1, 0));
			glm::quat qRoll = glm::angleAxis(glm::radians(pitch_yaw_roll.z), glm::vec3(0, 0, 1));
			cam_orientation = glm::normalize(qPitch * qYaw * qRoll);
		}

		void Rotate(glm::vec3 angle)
		{
			pitch_yaw_roll += angle;
			checkBorders();
			glm::quat qPitch = glm::angleAxis(glm::radians(pitch_yaw_roll.y), glm::vec3(1, 0, 0));
			glm::quat qYaw = glm::angleAxis(glm::radians(pitch_yaw_roll.x), glm::vec3(0, 1, 0));
			glm::quat qRoll = glm::angleAxis(glm::radians(pitch_yaw_roll.z), glm::vec3(0, 0, 1));
			cam_orientation = glm::normalize(qPitch * qYaw * qRoll);
		}

		void Rotate(glm::quat angle)
		{
			pitch_yaw_roll += glm::eulerAngles(angle);
			cam_orientation += angle;
		}

		void SetRotation(float pitch, float yaw, float roll)
		{
			pitch_yaw_roll = glm::vec3{ pitch, yaw, roll };
			glm::quat qPitch = glm::angleAxis(glm::radians(pitch_yaw_roll.y), glm::vec3(1, 0, 0));
			glm::quat qYaw = glm::angleAxis(glm::radians(pitch_yaw_roll.x), glm::vec3(0, 1, 0));
			glm::quat qRoll = glm::angleAxis(glm::radians(pitch_yaw_roll.z), glm::vec3(0, 0, 1));
			cam_orientation = glm::normalize(qPitch * qYaw * qRoll);
		}

		void SetRotation(glm::vec3 angle)
		{
			pitch_yaw_roll = angle;
			glm::quat qPitch = glm::angleAxis(glm::radians(pitch_yaw_roll.y), glm::vec3(1, 0, 0));
			glm::quat qYaw = glm::angleAxis(glm::radians(pitch_yaw_roll.x), glm::vec3(0, 1, 0));
			glm::quat qRoll = glm::angleAxis(glm::radians(pitch_yaw_roll.z), glm::vec3(0, 0, 1));
			cam_orientation = glm::normalize(qPitch * qYaw * qRoll);
		}

		void SetRotation(glm::quat angle)
		{
			pitch_yaw_roll = glm::eulerAngles(angle);
			cam_orientation = angle;
		}

		glm::quat& GetOrientation()
		{
			return cam_orientation;
		}

		void LockAxes(float pitch_up, float pitch_low, float yaw_up, float yaw_low, float roll_up, float roll_low)
		{
			bounds_up = glm::vec3(pitch_up, yaw_up, roll_up);
			bounds_low = glm::vec3(pitch_low, yaw_low, roll_low);
			axes_lock = true;
		}

		void LockAxes(glm::vec3 upper_borders, glm::vec3 lower_borders)
		{
			bounds_up = upper_borders;
			bounds_low = lower_borders;
			axes_lock = true;
		}

		void UnlockAxes()
		{
			axes_lock = false;
		}

		void MoveCamera(float x, float y, float z)
		{
			cam_pos += glm::vec3(x, y, z);
		}

		void MoveCamera(glm::vec3 vector)
		{
			cam_pos += vector;
		}

		void PositionCameraAt(float x, float y, float z)
		{
			cam_pos = glm::vec3(x, y, z);
		}

		void PositionCameraAt(glm::vec3 vector)
		{
			cam_pos = vector;
		}

		glm::vec3& GetCameraPosition()
		{
			return cam_pos;
		}

		glm::quat& GetCameraOrientation()
		{
			return cam_orientation;
		}

	private:
		glm::vec3 cam_pos = { 0.f, 0.f, 0.f };
		glm::quat cam_orientation = { 0.f, 0.f, 0.f, 0.f };
		glm::vec3 pitch_yaw_roll = { 0.f, 0.f, 0.f };
		glm::vec3 bounds_up = { 0.f, 0.f, 0.f };
		glm::vec3 bounds_low = { 0.f, 0.f, 0.f };
		bool axes_lock = false;

		void checkBorders()
		{
			if (axes_lock)
			{
				pitch_yaw_roll.x = pitch_yaw_roll.x > bounds_up.x && bounds_up.x != 0 ? bounds_up.x : pitch_yaw_roll.x;
				pitch_yaw_roll.x = pitch_yaw_roll.x < bounds_low.x && bounds_low.x != 0 ? bounds_low.x : pitch_yaw_roll.x;
				pitch_yaw_roll.y = pitch_yaw_roll.y > bounds_up.y && bounds_up.y != 0 ? bounds_up.y : pitch_yaw_roll.y;
				pitch_yaw_roll.y = pitch_yaw_roll.y < bounds_low.y && bounds_low.y != 0 ? bounds_low.y : pitch_yaw_roll.y;
				pitch_yaw_roll.z = pitch_yaw_roll.z > bounds_up.z && bounds_up.z != 0 ? bounds_up.z : pitch_yaw_roll.z;
				pitch_yaw_roll.z = pitch_yaw_roll.z < bounds_low.z && bounds_low.z != 0 ? bounds_low.z : pitch_yaw_roll.z;
			}
		}
	};
}