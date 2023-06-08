#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Entity.h"

namespace GrEngine
{
	enum ProjectionType
	{
		Perspective,
		Orthographic
	};

	class DllExport Camera : public Entity
	{
	public:
		Camera() 
		{
			Type |= EntityType::CameraEntity;
			//static_cast<EntityIDProperty*>(properties[1])->SetPropertyValue(std::rand() % 10000 + 1000);
			static_cast<EntityOrientationProperty*>(properties[3])->SetCallback([](Entity* owner, EntityProperty* self)
				{
					std::vector<std::string> vals = Globals::SeparateString(self->ValueString(), ':');
					owner->SetRotation(std::stof(vals[0]), std::stof(vals[1]), std::stof(vals[2]));
				});
		};
		virtual ~Camera() {};

		void Rotate(const float& pitch, const float& yaw, const float& roll) override
		{
			pitch_yaw_roll += glm::vec3{ pitch, yaw, roll };
			checkBorders();
			glm::quat q = glm::quat_cast(glm::mat3(1.f));
			q = q * glm::angleAxis(glm::radians(pitch_yaw_roll.x), glm::vec3(1, 0, 0));
			q = q * glm::angleAxis(glm::radians(pitch_yaw_roll.y), glm::vec3(0, 1, 0));
			q = q * glm::angleAxis(glm::radians(pitch_yaw_roll.z), glm::vec3(0, 0, 1));
			obj_orientation_target = q;
		}

		void Rotate(const glm::vec3& angle) override
		{
			pitch_yaw_roll += angle;
			checkBorders();
			glm::quat q = glm::quat_cast(glm::mat3(1.f));
			q = q * glm::angleAxis(glm::radians(pitch_yaw_roll.x), glm::vec3(1, 0, 0));
			q = q * glm::angleAxis(glm::radians(pitch_yaw_roll.y), glm::vec3(0, 1, 0));
			q = q * glm::angleAxis(glm::radians(pitch_yaw_roll.z), glm::vec3(0, 0, 1));
			obj_orientation_target = q;
		}

		 void Rotate(const glm::quat& angle) override
		 {
			 glm::quat rot = *obj_orientation * angle;
			 //Get approximation of PYR degree values
			 glm::mat4 q = glm::mat4_cast(rot);
			 pitch_yaw_roll = glm::degrees(glm::vec3(glm::eulerAngles(glm::quat_cast(glm::inverse(q)))));
			 pitch_yaw_roll *= -1.f;
			 checkBorders();
			 obj_orientation_target += angle;
		 }

		 void SetRotation(const float& pitch, const float& yaw, const float& roll) override
		 {
			 pitch_yaw_roll = glm::vec3{ pitch, yaw, roll };
			 static_cast<EntityOrientationProperty*>(properties[3])->SetPropertyValue(pitch_yaw_roll);
			 obj_orientation_target = *obj_orientation;
		 };

		 virtual void SetRotation(const glm::vec3& angle) override
		 {
			 pitch_yaw_roll = angle;
			 static_cast<EntityOrientationProperty*>(properties[3])->SetPropertyValue(pitch_yaw_roll);
			 obj_orientation_target = *obj_orientation;
		 };

		 virtual void SetRotation(const glm::quat& angle) override
		 {
			 //Get approximation of PYR degree values
			 glm::mat4 q = glm::mat4_cast(angle);
			 pitch_yaw_roll = glm::degrees(glm::vec3(glm::eulerAngles(glm::quat_cast(glm::inverse(q)))));
			 pitch_yaw_roll *= -1.f;
			 static_cast<EntityOrientationProperty*>(properties[3])->SetPropertyValue(angle);
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
			 static_cast<Vector3fProperty*>(properties[2])->SetPropertyValue(x, y, z);
		 };

		 virtual void PositionObjectAt(const glm::vec3& vector) override
		 {
			 object_position_target = vector;
			 static_cast<Vector3fProperty*>(properties[2])->SetPropertyValue(vector);
		 };

		 virtual glm::vec3 GetObjectPosition() override
		 {
			 if (parent == nullptr)
			 {
				 return *object_origin;
			 }
			 else
			 {
				 return *object_origin + parent->GetObjectPosition();
			 }
		 };

		 virtual glm::quat GetObjectOrientation() override
		 {
			 return *obj_orientation;
		 };

		 glm::mat4 GetProjectionMatrix(float aspect, float near_plane, float far_plane)
		 {
			 glm::mat4 proj(0.f);
			 switch (prType)
			 {
			 case ProjectionType::Perspective:
				 proj = glm::perspective(perspFOV, aspect, near_plane, far_plane);
				 proj[1][1] *= -1;
				 break;
			 case ProjectionType::Orthographic:
				 proj = glm::ortho(aspect * orthLeft, aspect * orthRight, orthBottom, orthUp, -far_plane, far_plane);
				 proj[1][1] *= -1;
				 break;
			 }

			 return proj;
		 }

		 glm::mat4 GetViewMatrix()
		 {
			 return glm::translate(glm::mat4_cast(GetObjectOrientation()), -GetObjectPosition());
		 }

		 void SetProjectionType(ProjectionType new_type)
		 {
			 prType = new_type;
		 }

		 void UpdateOrthographicProjection(float left, float right, float bottom, float up)
		 {
			 orthLeft = left;
			 orthRight = right;
			 orthBottom = bottom;
			 orthUp = up;
		 }

		 void UpdatePerspectiveProjection(float fov)
		 {
			 perspFOV = glm::radians(fov);
		 }

		 void ClampAxes()
		 {
			 if (axes_lock)
			 {
				 pitch_yaw_roll.x = pitch_yaw_roll.x > bounds_up.x && bounds_up.x != 0 ? bounds_up.x : pitch_yaw_roll.x;
				 pitch_yaw_roll.x = pitch_yaw_roll.x < bounds_low.x&& bounds_low.x != 0 ? bounds_low.x : pitch_yaw_roll.x;
				 pitch_yaw_roll.y = pitch_yaw_roll.y > bounds_up.y && bounds_up.y != 0 ? bounds_up.y : pitch_yaw_roll.y;
				 pitch_yaw_roll.y = pitch_yaw_roll.y < bounds_low.y&& bounds_low.y != 0 ? bounds_low.y : pitch_yaw_roll.y;
				 pitch_yaw_roll.z = pitch_yaw_roll.z > bounds_up.z && bounds_up.z != 0 ? bounds_up.z : pitch_yaw_roll.z;
				 pitch_yaw_roll.z = pitch_yaw_roll.z < bounds_low.z&& bounds_low.z != 0 ? bounds_low.z : pitch_yaw_roll.z;
				 static_cast<EntityOrientationProperty*>(properties[3])->SetPropertyValue(pitch_yaw_roll);
			 }
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

		glm::vec3 GetInternalPYR()
		{
			return pitch_yaw_roll;
		}

		glm::vec3& UpdateCameraPosition(float smoothing_factor = 0.f)
		{
			smoothing_factor = smoothing_factor == 1.f ? FLT_EPSILON : smoothing_factor;
			static_cast<Vector3fProperty*>(properties[2])->SetPropertyValue(*object_origin + (object_position_target - *object_origin) * (1 - smoothing_factor));
			glm::vec3 pos = GetObjectPosition();
			return pos;
		}

		glm::quat& UpdateCameraOrientation(float smoothing_factor = 0.f)
		{
			smoothing_factor = smoothing_factor == 1.f ? FLT_EPSILON : smoothing_factor;
			static_cast<EntityOrientationProperty*>(properties[3])->SetPropertyValue(*obj_orientation + (obj_orientation_target - *obj_orientation) * (1 - smoothing_factor));
			return *obj_orientation;
		}

	private:
		glm::vec3 bounds_up = { 0.f, 0.f, 0.f };
		glm::vec3 bounds_low = { 0.f, 0.f, 0.f };
		glm::quat obj_orientation_target = { 0.f, 0.f, 0.f, 0.f };
		glm::vec3 object_position_target = { 0.f, 0.f, 0.f };
		bool axes_lock = false;
		ProjectionType prType = ProjectionType::Perspective;
		float orthLeft = -1.f, orthRight = 1.f, orthUp = 1.f, orthBottom = -1.f;
		float perspFOV = glm::radians(60.f);

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