#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Core/Globals.h"
#include "Core/Logger.h"
#include "Properties/Property.h"

namespace GrEngine
{
	class DllExport Entity
	{
	public:

		Entity()
		{
			char _buf[11];
			std::snprintf(_buf, sizeof(_buf), "1%03d%03d%03d", std::rand() % 255 + 1, std::rand() % 255 + 1, std::rand() % 255 + 1);

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
			pitch_yaw_roll += glm::vec3{ pitch, yaw, roll };
			static_cast<EntityOrientation*>(properties[3])->SetPropertyValue(pitch_yaw_roll.x, pitch_yaw_roll.y, pitch_yaw_roll.z);
		};

		virtual void Rotate(const glm::vec3& angle)
		{
			pitch_yaw_roll += angle;
			static_cast<EntityOrientation*>(properties[3])->SetPropertyValue(pitch_yaw_roll.x, pitch_yaw_roll.y, pitch_yaw_roll.z);
		};

		virtual void Rotate(const glm::quat& angle)
		{
			pitch_yaw_roll += glm::degrees(glm::eulerAngles(angle));
			static_cast<EntityOrientation*>(properties[3])->SetPropertyValue(pitch_yaw_roll.x, pitch_yaw_roll.y, pitch_yaw_roll.z);
		};

		virtual void SetRotation(const float& pitch, const float& yaw, const float& roll)
		{
			pitch_yaw_roll = glm::vec3{ pitch, yaw, roll };
			static_cast<EntityOrientation*>(properties[3])->SetPropertyValue(pitch_yaw_roll.x, pitch_yaw_roll.y, pitch_yaw_roll.z);
		};

		virtual void SetRotation(const glm::vec3& angle)
		{
			pitch_yaw_roll = angle;
			static_cast<EntityOrientation*>(properties[3])->SetPropertyValue(pitch_yaw_roll.x, pitch_yaw_roll.y, pitch_yaw_roll.z);
		};

		virtual void SetRotation(const glm::quat& angle)
		{
			pitch_yaw_roll = glm::degrees(glm::eulerAngles(angle));
			static_cast<EntityOrientation*>(properties[3])->SetPropertyValue(pitch_yaw_roll.x, pitch_yaw_roll.y, pitch_yaw_roll.z);
		};

		virtual void MoveObject(const float& x, const float& y, const float& z)
		{
			static_cast<EntityPosition*>(properties[2])->SetPropertyValue(*object_origin + glm::vec3(x, y, z));
		};

		virtual void MoveObject(const glm::vec3& vector)
		{
			static_cast<EntityPosition*>(properties[2])->SetPropertyValue(*object_origin + vector);
		};

		virtual void PositionObjectAt(const float& x, const float& y, const float& z)
		{
			static_cast<EntityPosition*>(properties[2])->SetPropertyValue(x, y, z);
		};

		virtual void PositionObjectAt(const glm::vec3& vector)
		{
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

		virtual glm::mat4 GetObjectTransformation()
		{
			return glm::translate(glm::mat4(1.f), GetObjectPosition()) * glm::mat4_cast(GetObjectOrientation());
		}

		template<typename T>
		T GetPropertyValue(const char* property_name, T default_value)
		{
			for (std::vector<EntityProperty*>::iterator itt = properties.begin(); itt != properties.end(); ++itt)
			{
				if ((*itt)->property_name == std::string(property_name))
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

		inline std::vector<EntityProperty*>& GetProperties()
		{
			return properties;
		}

		inline std::string& GetEntityType()
		{
			return Type;
		};

		inline const char* GetEntityNameTag() 
		{ 
			return (*obj_name).c_str(); 
		};

		inline void MakeStatic()
		{
			isPrivated = true;
		}

		inline bool& IsStatic()
		{
			return isPrivated;
		}

	protected:
		std::vector<EntityProperty*> properties;

		std::string* obj_name;
		UINT* obj_id;
		glm::quat* obj_orientation;
		glm::vec3* object_origin;
		std::string Type = "Entity";

		glm::vec3 pitch_yaw_roll = { 0.f, 0.f, 0.f };
		bool isPrivated = false;
	};
}