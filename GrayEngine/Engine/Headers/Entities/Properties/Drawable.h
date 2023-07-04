#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Entities/Entity.h"
#include "Engine/Headers/Virtual/Physics.h"

namespace GrEngine
{
	enum TextureType
	{
		Color,
		Normal,
		Height
	};

	struct ImageInfo
	{
		uint32_t width;
		uint32_t height;
		uint32_t channels;
	};

	struct Texture
	{
		ImageInfo srcInfo;
		std::string resource_name;
		bool initialized = false;
	};

	struct Vertex
	{
		glm::vec4 pos;
		glm::vec3 norm;
		glm::vec3 tang;
		glm::vec3 bitang;
		glm::vec4 color;
		glm::vec2 uv;
		uint32_t uv_index;

		Vertex()
		{
			pos = glm::vec4(0.f);
			norm = glm::vec3(0.f);
			tang = glm::vec3(0.f);
			bitang = glm::vec3(0.f);
			uv = glm::vec2(0.f);
		}

		Vertex(glm::vec4 position, glm::vec3 normal, glm::vec2 uv_coordinates, uint32_t material_index = 0)
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

		virtual bool LoadMesh(const char* mesh_path) = 0;

		virtual bool LoadModel(const char* mesh_path, std::vector<std::string> textures_vector, std::vector<std::string> normals_vector) = 0;

		virtual bool AssignTextures(std::vector<std::string> textures_vector) = 0;

		virtual bool AssignNormals(std::vector<std::string> normals_vector) = 0;

		virtual void GeneratePlaneMesh(float width, int subdivisions) = 0;

		virtual void GenerateBoxMesh(float width, float height, float depth) = 0;

		virtual void GenerateSphereMesh(double radius, int rings, int slices) = 0;

		virtual void RecalculateNormals();

		virtual void Refresh() = 0;

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

		virtual glm::vec3& GetObjectBounds()
		{
			return bound;
		};

		virtual void SetObjectBounds(glm::vec3 new_bounds)
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

		std::vector<std::string> GetTextureCollection()
		{
			return texture_names;
		};

		void DisableCollisions()
		{
			CollisionEnabled = false;
			ownerEntity->ParsePropertyValue("Mass", "0");

			GrEngine::PhysicsObject* physComponent = (GrEngine::PhysicsObject*)ownerEntity->GetPropertyValue(PropertyType::PhysComponent, (void*)nullptr);
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

		std::vector<std::string> texture_names;
		std::string mesh_name;

	protected:

		Entity* ownerEntity = nullptr;
		glm::vec3 bound = { 0.f, 0.f, 0.f };
		bool visibility = true;
		bool CollisionEnabled = true;
	};
};