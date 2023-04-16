#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Entities/Entity.h"
#include "Engine/Headers/Virtual/Physics.h"

namespace GrEngine
{
	struct Texture
	{
		std::vector<std::string> texture_collection;
		bool initialized = false;
	};

	struct Vertex
	{
		glm::vec4 pos;
		glm::vec4 norm;
		glm::vec4 tang;
		glm::vec4 color;
		glm::vec2 uv;
		uint32_t uv_index;

		Vertex()
		{
			pos = glm::vec4(0.f);
			norm = glm::vec4(0.f);
			tang = glm::vec4(0.f);
			uv = glm::vec2(0.f);
		}

		Vertex(glm::vec4 position, glm::vec4 normal, glm::vec2 uv_coordinates, uint32_t material_index = 0)
		{
			pos = position;
			uv = uv_coordinates;
			uv_index = material_index;
			norm = normal;
		}

		bool operator==(const Vertex& other) const
		{
			return pos == other.pos && uv == other.uv;
		}
	};

	struct Mesh
	{
		std::string mesh_path = "";
	};

	struct PixelData
	{
		unsigned char* data;
		int width;
		int height;
		int channels;
	};

	class Object
	{
	public:

		Object(Entity* owner)
		{
			ownerEntity = owner;
		};

		virtual ~Object()
		{

		};

		virtual bool LoadMesh(const char* mesh_path, std::vector<std::string>* out_materials) = 0;

		virtual bool LoadModel(const char* model_path) = 0;

		virtual bool LoadModel(const char* mesh_path, std::vector<std::string> textures_vector) = 0;

		virtual void GeneratePlaneMesh(float width, int subdivisions) = 0;

		virtual void GenerateBoxMesh(float width, float height, float depth) = 0;

		virtual void Refresh() = 0;

		virtual void CalculateNormals() = 0;

		virtual glm::vec3 GetObjectPosition()
		{
			return ownerEntity->GetObjectPosition();
		};

		virtual glm::quat GetObjectOrientation()
		{
			return ownerEntity->GetObjectOrientation();
		};

		virtual glm::mat4 GetObjectTransformation()
		{
			return glm::translate(glm::mat4(1.f), GetObjectPosition()) * glm::mat4_cast(GetObjectOrientation());
		};

		virtual glm::uvec3& GetObjectBounds()
		{
			return bound;
		};

		virtual void SetObjectBounds(glm::uvec3 new_bounds)
		{
			bound = new_bounds;
		};

		bool& IsVisible()
		{
			return visibility;
		};

		void SetVisisibility(bool value)
		{
			visibility = value;
		};

		const EntityType& GetEntityType()
		{
			return ownerEntity->GetEntityType();
		};

		std::unordered_map<std::string, std::string> GetMaterials()
		{
			std::unordered_map<std::string, std::string> res;

			for (int i = 0; i < material_names.size(); i++)
			{
				if (i < texture_names.size())
					res.insert_or_assign(material_names[i], texture_names[i]);
				else
					res.insert_or_assign(material_names[i], "nil");
			}

			return res;
		};

		virtual void updateCollisions() = 0;

		void recalculatePhysics(bool enabled)
		{
			GrEngine::PhysicsObject* physComponent = ownerEntity->GetPropertyValue(PropertyType::PhysComponent, static_cast<GrEngine::PhysicsObject*>(nullptr));
			if (physComponent != nullptr)
			{
				physComponent->CalculatePhysics();
			}
		};

		void DisableCollisions()
		{
			CollisionEnabled = false;
			ownerEntity->ParsePropertyValue("Mass", "0");

			GrEngine::PhysicsObject* physComponent = ownerEntity->GetPropertyValue(PropertyType::PhysComponent, static_cast<GrEngine::PhysicsObject*>(nullptr));
			if (physComponent != nullptr)
			{
				physComponent->DisablePhysics();
			}
		};

		Entity* GetOwnerEntity()
		{
			return ownerEntity;
		};

		static GrEngine::Object* FindObject(GrEngine::Entity* entity)
		{
			EntityProperty* ent_property = entity->GetProperty(PropertyType::Drawable);
			if (ent_property != nullptr)
			{
				return static_cast<GrEngine::Object*>(ent_property->GetValueAdress());
			}
			else
			{
				return nullptr;
			}
		};

		std::vector<std::string> material_names;
		std::vector<std::string> texture_names;

	protected:

		Entity* ownerEntity = nullptr;
		glm::uvec3 bound = { 0.f, 0.f, 0.f };
		bool visibility = true;
		bool CollisionEnabled = true;
	};
};