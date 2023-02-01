#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Entity.h"

namespace GrEngine
{
	class DllExport Camera : public Entity
	{
	public:
		Camera() 
		{
			glm::quat qPitch = glm::angleAxis(glm::radians(pitch_yaw_roll.y), glm::vec3(1, 0, 0));
			glm::quat qYaw = glm::angleAxis(glm::radians(pitch_yaw_roll.x), glm::vec3(0, 1, 0));
			glm::quat qRoll = glm::angleAxis(glm::radians(pitch_yaw_roll.z), glm::vec3(0, 0, 1));
			obj_orientation_target = glm::normalize(qPitch * qYaw * qRoll);
			static_cast<EntityOrientation*>(properties[3])->SetPropertyValue(glm::normalize(qPitch * qYaw * qRoll));

			Type = "Camera";
			static_cast<EntityID*>(properties[1])->SetPropertyValue(std::rand() % 10000 + 1000);
		};
		virtual ~Camera() {};

		void Rotate(const float& pitch, const float& yaw, const float& roll) override
		{
			pitch_yaw_roll += glm::vec3{ pitch, yaw, roll };
			checkBorders();
			glm::quat qPitch = glm::angleAxis(glm::radians(pitch_yaw_roll.y), glm::vec3(1, 0, 0));
			glm::quat qYaw = glm::angleAxis(glm::radians(pitch_yaw_roll.x), glm::vec3(0, 1, 0));
			glm::quat qRoll = glm::angleAxis(glm::radians(pitch_yaw_roll.z), glm::vec3(0, 0, 1));
			obj_orientation_target = glm::normalize(qPitch * qYaw * qRoll);
		}

		void Rotate(const glm::vec3& angle) override
		{
			pitch_yaw_roll += angle;
			checkBorders();
			glm::quat qPitch = glm::angleAxis(glm::radians(pitch_yaw_roll.y), glm::vec3(1, 0, 0));
			glm::quat qYaw = glm::angleAxis(glm::radians(pitch_yaw_roll.x), glm::vec3(0, 1, 0));
			glm::quat qRoll = glm::angleAxis(glm::radians(pitch_yaw_roll.z), glm::vec3(0, 0, 1));
			obj_orientation_target = glm::normalize(qPitch * qYaw * qRoll);
		}

		 void Rotate(const glm::quat& angle) override
		 {
			 pitch_yaw_roll += glm::eulerAngles(angle) * glm::vec3(Globals::delta_time, Globals::delta_time, Globals::delta_time);
			 checkBorders();
			 obj_orientation_target += angle;
		 }

		 void SetRotation(const float& pitch, const float& yaw, const float& roll) override
		 {
			 pitch_yaw_roll = glm::vec3{ pitch, yaw, roll };
			 glm::quat qPitch = glm::angleAxis(glm::radians(yaw), glm::vec3(1, 0, 0));
			 glm::quat qYaw = glm::angleAxis(glm::radians(pitch), glm::vec3(0, 1, 0));
			 glm::quat qRoll = glm::angleAxis(glm::radians(roll), glm::vec3(0, 0, 1));
			 obj_orientation_target = glm::normalize(qPitch * qYaw * qRoll);
			 static_cast<EntityOrientation*>(properties[3])->SetPropertyValue(pitch_yaw_roll.x, pitch_yaw_roll.y, pitch_yaw_roll.z);
		 };

		 virtual void SetRotation(const glm::vec3& angle) override
		 {
			 pitch_yaw_roll = angle;
			 glm::quat qPitch = glm::angleAxis(glm::radians(angle.y), glm::vec3(1, 0, 0));
			 glm::quat qYaw = glm::angleAxis(glm::radians(angle.x), glm::vec3(0, 1, 0));
			 glm::quat qRoll = glm::angleAxis(glm::radians(angle.z), glm::vec3(0, 0, 1));
			 obj_orientation_target = glm::normalize(qPitch * qYaw * qRoll);
			 static_cast<EntityOrientation*>(properties[3])->SetPropertyValue(pitch_yaw_roll.x, pitch_yaw_roll.y, pitch_yaw_roll.z);
		 };

		 virtual void SetRotation(const glm::quat& angle) override
		 {
			 pitch_yaw_roll = glm::eulerAngles(angle);
			 static_cast<EntityOrientation*>(properties[3])->SetPropertyValue(angle);
			 obj_orientation_target = angle;
		 };

		 virtual void MoveObject(const float& x, const float& y, const float& z) override
		 {
			 object_position_target += glm::vec3(x * Globals::delta_time, y * Globals::delta_time, z * Globals::delta_time);
		 };

		 virtual void MoveObject(const glm::vec3& vector) override
		 {
			 object_position_target += vector * glm::vec3(Globals::delta_time, Globals::delta_time, Globals::delta_time);
		 };

		 virtual void PositionObjectAt(const float& x, const float& y, const float& z) override
		 {
			 object_position_target = glm::vec3(x, y, z);
			 static_cast<EntityPosition*>(properties[2])->SetPropertyValue(x, y, z);
		 };

		 virtual void PositionObjectAt(const glm::vec3& vector) override
		 {
			 object_position_target = vector;
			 static_cast<EntityPosition*>(properties[2])->SetPropertyValue(vector);
		 };

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

		glm::vec3& UpdateCameraPosition(float smoothing_factor = 0.f)
		{
			smoothing_factor = smoothing_factor == 1.f ? FLT_EPSILON : smoothing_factor;
			static_cast<EntityPosition*>(properties[2])->SetPropertyValue(*object_origin + (object_position_target - *object_origin) * (1 - smoothing_factor));
			return *object_origin;
		}

		glm::quat& UpdateCameraOrientation(float smoothing_factor = 0.f)
		{
			smoothing_factor = smoothing_factor == 1.f ? FLT_EPSILON : smoothing_factor;
			static_cast<EntityOrientation*>(properties[3])->SetPropertyValue(*obj_orientation + (obj_orientation_target - *obj_orientation) * (1 - smoothing_factor));
			return *obj_orientation;
		}

	private:
		glm::vec3 bounds_up = { 0.f, 0.f, 0.f };
		glm::vec3 bounds_low = { 0.f, 0.f, 0.f };
		glm::quat obj_orientation_target = { 0.f, 0.f, 0.f, 0.f };
		glm::vec3 object_position_target = { 0.f, 0.f, 0.f };
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