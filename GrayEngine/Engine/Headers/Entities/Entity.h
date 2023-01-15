#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Core/Globals.h"
#include "Core/Logger.h"
#include "Properties/Property.h"
#include "Virtual/Physics.h"

namespace GrEngine
{
	class DllExport Entity
	{
	public:

		Entity()
		{
			char buf[11];
			char _buf[11];
			std::snprintf(_buf, sizeof(_buf), "1%03d%03d%03d", next_id[0] + 1, next_id[1] + 1, next_id[2]++ + 1);

			next_id[0] += next_id[1] / 254;
			next_id[1] += next_id[2] / 254;
			next_id[2] %= 254;

			//Don't shuffle
			properties.push_back(new EntityName("Object", this));
			properties.push_back(new EntityID(atoi(_buf), this));
			properties.push_back(new EntityPosition(0.f, 0.f, 0.f, this));
			properties.push_back(new EntityOrientation(0.f, 0.f, 0.f, this));
			//Don't shuffle

			obj_name = static_cast<std::string*>(properties[0]->GetValueAdress());
			obj_id = static_cast<UINT*>(properties[1]->GetValueAdress());
			object_origin = static_cast<glm::vec3*>(properties[2]->GetValueAdress());
			obj_orientation = static_cast<glm::quat*>(properties[3]->GetValueAdress());
		};

		Entity(UINT id)
		{
			//Don't shuffle
			properties.push_back(new EntityName("Object", this));
			properties.push_back(new EntityID(id, this));
			properties.push_back(new EntityPosition(0.f, 0.f, 0.f, this));
			properties.push_back(new EntityOrientation(0.f, 0.f, 0.f, this));
			//Don't shuffle

			obj_name = static_cast<std::string*>(properties[0]->GetValueAdress());
			obj_id = static_cast<UINT*>(properties[1]->GetValueAdress());
			object_origin = static_cast<glm::vec3*>(properties[2]->GetValueAdress());
			obj_orientation = static_cast<glm::quat*>(properties[3]->GetValueAdress());

			next_id = { (short)(id / 1000000 % 1000), (short)(id / 1000 % 1000), (short)(id % 1000) };
		};

		virtual ~Entity()
		{
			for (std::vector<EntityProperty*>::iterator itt = properties.begin(); itt != properties.end(); ++itt)
			{
				delete (*itt);
			}
		};

		virtual void Rotate(const float& pitch, const float& yaw, const float& roll)
		{
			pitch_yaw_roll += glm::vec3{ pitch * Globals::delta_time, yaw * Globals::delta_time, roll * Globals::delta_time };
			glm::quat qPitch = glm::angleAxis(glm::radians(pitch_yaw_roll.y), glm::vec3(1, 0, 0));
			glm::quat qYaw = glm::angleAxis(glm::radians(pitch_yaw_roll.x), glm::vec3(0, 1, 0));
			glm::quat qRoll = glm::angleAxis(glm::radians(pitch_yaw_roll.z), glm::vec3(0, 0, 1));
			obj_orientation_target = glm::normalize(qPitch * qYaw * qRoll);
		};

		virtual void Rotate(const glm::vec3& angle)
		{
			pitch_yaw_roll += angle * glm::vec3(Globals::delta_time, Globals::delta_time, Globals::delta_time);
			glm::quat qPitch = glm::angleAxis(glm::radians(pitch_yaw_roll.y), glm::vec3(1, 0, 0));
			glm::quat qYaw = glm::angleAxis(glm::radians(pitch_yaw_roll.x), glm::vec3(0, 1, 0));
			glm::quat qRoll = glm::angleAxis(glm::radians(pitch_yaw_roll.z), glm::vec3(0, 0, 1));
			obj_orientation_target = glm::normalize(qPitch * qYaw * qRoll);
		};

		virtual void Rotate(const glm::quat& angle)
		{
			pitch_yaw_roll += glm::eulerAngles(angle) * glm::vec3(Globals::delta_time, Globals::delta_time, Globals::delta_time);
			obj_orientation_target += angle;
		};

		virtual void SetRotation(const float& pitch, const float& yaw, const float& roll)
		{
			pitch_yaw_roll = glm::vec3{ pitch, yaw, roll };
			glm::quat qPitch = glm::angleAxis(glm::radians(yaw), glm::vec3(1, 0, 0));
			glm::quat qYaw = glm::angleAxis(glm::radians(pitch), glm::vec3(0, 1, 0));
			glm::quat qRoll = glm::angleAxis(glm::radians(roll), glm::vec3(0, 0, 1));
			static_cast<EntityOrientation*>(properties[3])->SetPropertyValue(glm::normalize(qPitch * qYaw * qRoll));
			obj_orientation_target = glm::normalize(qPitch * qYaw * qRoll);
		};

		virtual void SetRotation(const glm::vec3& angle)
		{
			pitch_yaw_roll = angle;
			glm::quat qPitch = glm::angleAxis(glm::radians(angle.y), glm::vec3(1, 0, 0));
			glm::quat qYaw = glm::angleAxis(glm::radians(angle.x), glm::vec3(0, 1, 0));
			glm::quat qRoll = glm::angleAxis(glm::radians(angle.z), glm::vec3(0, 0, 1));
			static_cast<EntityOrientation*>(properties[3])->SetPropertyValue(glm::normalize(qPitch * qYaw * qRoll));
			obj_orientation_target = glm::normalize(qPitch * qYaw * qRoll);
		};

		virtual void SetRotation(const glm::quat& angle)
		{
			pitch_yaw_roll = glm::eulerAngles(angle);
			static_cast<EntityOrientation*>(properties[3])->SetPropertyValue(angle);
			obj_orientation_target = angle;
		};

		virtual void MoveObject(const float& x, const float& y, const float& z)
		{
			object_position_target += glm::vec3(x * Globals::delta_time, y * Globals::delta_time, z * Globals::delta_time);
		};

		virtual void MoveObject(const glm::vec3& vector)
		{
			object_position_target += vector * glm::vec3(Globals::delta_time, Globals::delta_time, Globals::delta_time);
		};

		virtual void PositionObjectAt(const float& x, const float& y, const float& z)
		{
			object_position_target = glm::vec3(x, y, z);
			static_cast<EntityPosition*>(properties[2])->SetPropertyValue(x, y, z);
		};

		virtual void PositionObjectAt(const glm::vec3& vector)
		{
			object_position_target = vector;
			static_cast<EntityPosition*>(properties[2])->SetPropertyValue(vector);
		};

		virtual glm::vec3 GetObjectPosition()
		{
			return *object_origin;
		};

		virtual glm::quat GetObjectOrientation()
		{
			return *obj_orientation;
		};

		template<typename T>
		T GetPropertyValue(const char* property_name, T default_value)
		{
			for (std::vector<EntityProperty*>::iterator itt = properties.begin(); itt != properties.end(); ++itt)
			{
				if ((*itt)->property_name == property_name)
				{
					return std::any_cast<T>((*itt)->GetAnyValue());
				}
			}

			return default_value;
		};

		inline EntityProperty* GetProperty(const char* property_name)
		{
			for (std::vector<EntityProperty*>::iterator itt = properties.begin(); itt != properties.end(); ++itt)
			{
				if ((*itt)->property_name == std::string(property_name))
				{
					return (*itt);
				}
			}

			return nullptr;
		};

		inline bool HasProperty(const char* property_name)
		{
			for (std::vector<EntityProperty*>::iterator itt = properties.begin(); itt != properties.end(); ++itt)
			{
				if ((*itt)->property_name == std::string(property_name))
				{
					return true;
				}
			}

			return false;
		};

		virtual void ParsePropertyValue(const char* property_name, const char* property_value)
		{
			for (std::vector<EntityProperty*>::iterator itt = properties.begin(); itt != properties.end(); ++itt)
			{
				if ((*itt)->property_name == std::string(property_name))
				{
					(*itt)->ParsePropertyValue(property_value);
				}
			}
		};

		void* FindPropertyAdress(const char* property_name)
		{
			for (std::vector<EntityProperty*>::iterator itt = properties.begin(); itt != properties.end(); ++itt)
			{
				if ((*itt)->property_name == std::string(property_name))
				{
					return (*itt)->GetValueAdress();
				}
			}

			return nullptr;
		};

		inline void UpdateNameTag(std::string new_name)
		{
			static_cast<EntityName*>(properties[0])->SetPropertyValue(new_name.c_str());
		};

		inline std::string& GetObjectName()
		{
			return *obj_name;
		};

		inline UINT& GetEntityID()
		{
			return *obj_id;
		};

		bool AddNewProperty(const char* property_name)
		{
			if (std::string(property_name) == "Mass")
			{
				properties.push_back(new Mass(0.f, this));
				return true;
			}
			else if (std::string(property_name) == "Mesh" || std::string(property_name) == "Drawable")
			{
				properties.push_back(new Drawable("", this));
				return true;
			}
			else if (std::string(property_name) == "Scale")
			{
				properties.push_back(new Scale(1.f, 1.f, 1.f, this));
				return true;
			}
			else if (std::string(property_name) == "Color")
			{
				properties.push_back(new Color(1.f, 1.f, 1.f, this));
				return true;
			}
			else
			{
				return false;
			}
		};

		std::vector<EntityProperty*>& GetProperties()
		{
			return properties;
		}

		std::string GetEntityType()
		{
			return Type;
		};

		inline const char* GetEntityNameTag() { return (*obj_name).c_str(); };


	protected:
		std::vector<EntityProperty*> properties;

		std::string* obj_name;
		UINT* obj_id;
		glm::quat* obj_orientation;
		glm::vec3* object_origin;
		std::string Type = "Entity";

		glm::quat obj_orientation_target = { 0.f, 0.f, 0.f, 0.f };
		glm::vec3 object_position_target = { 0.f, 0.f, 0.f };
		glm::vec3 pitch_yaw_roll = { 0.f, 0.f, 0.f };
	private:
		static std::array<short, 3> next_id;
	};
}