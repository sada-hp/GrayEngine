#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Core/Globals.h"
#include "Core/Logger.h"
#include "Properties/Property.h"
#include "Virtual/Physics.h"

namespace GrEngine
{
	struct EntityInfo
	{
		std::string EntityName;
		UINT EntityID;
		glm::vec3 Position;
		glm::vec3 Orientation;
		glm::vec3 Scale;
	};

	class DllExport Entity
	{
	public:

		Entity() 
		{
			char buf[11];
			char _buf[11];
			std::snprintf(_buf, sizeof(_buf), "1%03d%03d%03d", std::rand() % 255 + 1, std::rand() % 255 + 1, std::rand() % 255 + 1);

			//Don't shuffle
			properties.push_back(new EntityName("Object", this));
			properties.push_back(new EntityID(atoi(_buf), this));
			properties.push_back(new EntityPosition(0.f, 0.f, 0.f, this));
			properties.push_back(new EntityOrientation(0.f, 0.f, 0.f, this));
			//Don't shuffle

			object_origin = static_cast<glm::vec3*>(properties[2]->GetValueAdress());
			obj_orientation = static_cast<glm::quat*>(properties[3]->GetValueAdress());

			recalculatePhysics();
		}

		virtual ~Entity() 
		{

		}

		virtual void Rotate(const float& pitch, const float& yaw, const float& roll)
		{
			pitch_yaw_roll += glm::vec3{ pitch * Globals::delta_time, yaw * Globals::delta_time, roll * Globals::delta_time };
			glm::quat qPitch = glm::angleAxis(glm::radians(pitch_yaw_roll.y), glm::vec3(1, 0, 0));
			glm::quat qYaw = glm::angleAxis(glm::radians(pitch_yaw_roll.x), glm::vec3(0, 1, 0));
			glm::quat qRoll = glm::angleAxis(glm::radians(pitch_yaw_roll.z), glm::vec3(0, 0, 1));
			obj_orientation_target = glm::normalize(qPitch * qYaw * qRoll);
		}

		virtual void Rotate(const glm::vec3& angle)
		{
			pitch_yaw_roll += angle * glm::vec3(Globals::delta_time, Globals::delta_time, Globals::delta_time);
			glm::quat qPitch = glm::angleAxis(glm::radians(pitch_yaw_roll.y), glm::vec3(1, 0, 0));
			glm::quat qYaw = glm::angleAxis(glm::radians(pitch_yaw_roll.x), glm::vec3(0, 1, 0));
			glm::quat qRoll = glm::angleAxis(glm::radians(pitch_yaw_roll.z), glm::vec3(0, 0, 1));
			obj_orientation_target = glm::normalize(qPitch * qYaw * qRoll);
		}

		virtual void Rotate(const glm::quat& angle)
		{
			pitch_yaw_roll += glm::eulerAngles(angle) * glm::vec3(Globals::delta_time, Globals::delta_time, Globals::delta_time);
			obj_orientation_target += angle;
		}

		virtual void SetRotation(const float& pitch, const float& yaw, const float& roll)
		{
			pitch_yaw_roll = glm::vec3{ pitch, yaw, roll };
			glm::quat qPitch = glm::angleAxis(glm::radians(yaw), glm::vec3(1, 0, 0));
			glm::quat qYaw = glm::angleAxis(glm::radians(pitch), glm::vec3(0, 1, 0));
			glm::quat qRoll = glm::angleAxis(glm::radians(roll), glm::vec3(0, 0, 1));
			static_cast<EntityOrientation*>(properties[3])->SetPropertyValue(glm::normalize(qPitch * qYaw * qRoll));
			obj_orientation_target = glm::normalize(qPitch * qYaw * qRoll);
		}

		virtual void SetRotation(const glm::vec3& angle)
		{
			pitch_yaw_roll = angle;
			glm::quat qPitch = glm::angleAxis(glm::radians(angle.y), glm::vec3(1, 0, 0));
			glm::quat qYaw = glm::angleAxis(glm::radians(angle.x), glm::vec3(0, 1, 0));
			glm::quat qRoll = glm::angleAxis(glm::radians(angle.z), glm::vec3(0, 0, 1));
			static_cast<EntityOrientation*>(properties[3])->SetPropertyValue(glm::normalize(qPitch * qYaw * qRoll));
			obj_orientation_target = glm::normalize(qPitch * qYaw * qRoll);
		}

		virtual void SetRotation(const glm::quat& angle)
		{
			pitch_yaw_roll = glm::eulerAngles(angle);
			static_cast<EntityOrientation*>(properties[3])->SetPropertyValue(angle);
			obj_orientation_target = angle;
		}

		virtual void MoveObject(const float& x, const float& y, const float& z)
		{
			object_position_target += glm::vec3(x * Globals::delta_time, y * Globals::delta_time, z * Globals::delta_time);
		}

		virtual void MoveObject(const glm::vec3& vector)
		{
			object_position_target += vector * glm::vec3(Globals::delta_time, Globals::delta_time, Globals::delta_time);
		}

		virtual void PositionObjectAt(const float& x, const float& y, const float& z)
		{
			object_position_target = glm::vec3(x, y, z);
			static_cast<EntityPosition*>(properties[2])->SetPropertyValue(x, y, z);
		}

		virtual void PositionObjectAt(const glm::vec3& vector)
		{
			object_position_target = vector;
			static_cast<EntityPosition*>(properties[2])->SetPropertyValue(vector);
		}

		virtual glm::vec3& GetObjectPosition()
		{
			if (Physics::GetContext()->GetSimulationState())
			{
				std::string mass_property = GetPropertyValue("Mass");
				float obj_mass = mass_property == "" ? 0.f : std::stof(mass_property);

				if (obj_mass != 0.f)
				{
					auto phys_pos = body->getWorldTransform().getOrigin();
					auto pos = glm::vec3(phys_pos.x(), phys_pos.y(), phys_pos.z());;
					return pos;
				}
				else
				{
					return *object_origin;
				}
			}
			else
			{
				return *object_origin;
			}
		}

		virtual glm::quat& GetObjectOrientation()
		{
			if (physics_object && Physics::GetContext()->GetSimulationState())
			{
				std::string mass_property = GetPropertyValue("Mass");
				float obj_mass = mass_property == "" ? 0.f : std::stof(mass_property);

				if (obj_mass != 0.f)
				{
					auto phys_pos = body->getWorldTransform().getRotation();
					auto ori = glm::quat(phys_pos.x(), phys_pos.y(), phys_pos.z(), phys_pos.w()) * *obj_orientation;
					return ori;
				}
				else
				{
					return *obj_orientation;
				}
			}

			return *obj_orientation;
		}

		virtual glm::vec3& GetObjectScale()
		{
			return scale;
		}

		virtual void SetObjectScale(glm::vec3 new_scale)
		{
			scale = new_scale;
		}

		virtual void SetObjectScale(float scalex, float scaley, float scalez)
		{
			scale = { scalex, scaley, scalez };
		}

		inline std::string GetPropertyValue(const char* property_name)
		{
			for (int i = 0; i < properties.size(); i++)
			{
				if (properties[i]->property_name == property_name)
				{
					return properties[i]->ValueString();
				}
			}
			
			return "";
		}

		inline bool HasProperty(const char* property_name)
		{
			for (int i = 0; i < properties.size(); i++)
			{
				auto name = properties[i]->property_name;
				if (properties[i]->property_name == std::string(property_name))
				{
					return true;
				}
			}

			return false;
		}

		void ParsePropertyValue(const char* property_name, const char* property_value)
		{
			for (int i = 0; i < properties.size(); i++)
			{
				auto name = properties[i]->property_name;
				if (std::string(properties[i]->property_name) == std::string(property_name))
				{
					properties[i]->ParsePropertyValue(property_value);
				}
			}
		}

		void* FindPropertyAdress(const char* property_name)
		{
			for (int i = 0; i < properties.size(); i++)
			{
				auto name = properties[i]->property_name;
				if (std::string(properties[i]->property_name) == std::string(property_name))
				{
					return properties[i]->GetValueAdress();
				}
			}

			return nullptr;
		}

		inline void UpdateNameTag(std::string new_name) 
		{ 
			ParsePropertyValue("EntityName", new_name.c_str());
		}

		inline UINT GetEntityID()
		{
			return static_cast<EntityID*>(properties[1])->GetValue();
		}

		void recalculatePhysics(bool rewrite = false)
		{
			std::string mass_property = GetPropertyValue("Mass");
			float obj_mass = mass_property == "" ? 0.f : std::stof(mass_property);

			if (obj_mass == 0.f && body != nullptr && !rewrite || !physics_object) return;

			if (body != nullptr)
			{
				Physics::GetContext()->RemoveObject(static_cast<void*>(body));
				delete body;
				delete myMotionState;
			}

			btTransform startTransform;
			startTransform.setIdentity();
			glm::vec3 pos = std::any_cast<glm::vec3>(properties[2]->GetAnyValue());
			startTransform.setOrigin(btVector3(pos.x, pos.y, pos.z));
			startTransform.setRotation(btQuaternion(btVector3(1, 0, 0), M_PI));
			btVector3 localInertia{ 0,0,0 };

			if (obj_mass != 0.f && physics_object)
				colShape->calculateLocalInertia(obj_mass, localInertia);

			colShape->setLocalScaling(btVector3(scale.x, -1 * scale.y, scale.z));

			myMotionState = new btDefaultMotionState(startTransform);
			btRigidBody::btRigidBodyConstructionInfo rbInfo(obj_mass, myMotionState, colShape, localInertia);
			rbInfo.m_angularDamping = .2f;
			rbInfo.m_linearDamping = .2f;
			body = new btRigidBody(rbInfo);

			Physics::GetContext()->AddSimulationObject(static_cast<void*>(body));
		}

		std::string GetEntityType()
		{
			return Type;
		};

		inline const char* GetEntityNameTag() { return properties[0]->ValueString(); };

		std::vector<EntityProperty*> properties;

	protected:
		glm::quat* obj_orientation;
		glm::vec3* object_origin;
		std::string Type = "Entity";

		glm::quat obj_orientation_target = { 0.f, 0.f, 0.f, 0.f };
		glm::vec3 object_position_target = { 0.f, 0.f, 0.f };
		glm::vec3 pitch_yaw_roll = { 0.f, 0.f, 0.f };

		glm::vec3 scale = { 1.f, 1.f, 1.f };
		

		btCollisionShape* colShape = new btBoxShape(btVector3(0.25f, 0.25f, 0.25f));
		btRigidBody* body;
		btDefaultMotionState* myMotionState;
		btTriangleMesh* colMesh;
		bool physics_object = false;

	public:
		void AddNewProperty(const char* property_name)
		{
			if (std::string(property_name) == "Mass")
			{
				properties.push_back(new Mass(0.f, this));
			}
			else if (std::string(property_name) == "Mesh")
			{
				properties.push_back(new Drawable("", this));
			}
		}
	};
}