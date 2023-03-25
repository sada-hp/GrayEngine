#pragma once
#include <pch.h>
#include "Entity.h"
#include "Engine/Headers/Engine.h"
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
			tang = glm::vec4(0.f);
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

	class DllExport DrawableObject : public Entity
	{
	public:
		DrawableObject()
		{
			Type = "Object";
		};

		DrawableObject(UINT id) : Entity(id)
		{
			Type = "Object";
		};

		virtual ~DrawableObject()
		{
			if (physComponent != nullptr)
			{
				delete physComponent;
			}
		};

		virtual bool LoadMesh(const char* mesh_path, std::vector<std::string>* out_materials) = 0;
		virtual bool LoadModel(const char* model_path) = 0;
		virtual bool LoadModel(const char* mesh_path, std::vector<std::string> textures_vector) = 0;
		virtual void GeneratePlaneMesh(float width, int subdivisions) = 0;
		virtual void GenerateBoxMesh(float width, float height, float depth) = 0;
		virtual void Refresh() = 0;
		virtual void CalculateNormals() = 0;

		virtual glm::vec3 GetObjectPosition() override
		{
			if (Engine::GetContext()->GetPhysics()->GetSimulationState() && physComponent->HasValue())
			{
				auto pos = physComponent->GetPhysPosition();
				return pos;
			}
			else
			{
				return *object_origin;
			}
		};

		virtual glm::quat GetObjectOrientation() override
		{
			if (Engine::GetContext()->GetPhysics()->GetSimulationState() && physComponent->HasValue())
			{
				auto ori = physComponent->GetPhysOrientation();
				return ori;
			}
			else
			{
				return *obj_orientation;
			}
		};

		void ParsePropertyValue(const char* property_name, const char* property_value) override
		{
			for (std::vector<EntityProperty*>::iterator itt = properties.begin(); itt != properties.end(); ++itt)
			{
				if ((*itt)->property_name == std::string(property_name))
				{
					(*itt)->ParsePropertyValue(property_value);
				}
			}
		}


		virtual glm::uvec3& GetObjectBounds()
		{
			return bound;
		}

		virtual void SetObjectBounds(glm::uvec3 new_bounds)
		{
			bound = new_bounds;
		}

		bool& IsVisible()
		{
			return visibility;
		}

		void SetVisisibility(bool value)
		{
			visibility = value;
		}

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
		}

		void recalculatePhysics(bool enabled)
		{
			physComponent->CalculatePhysics();
		};

		void DisableCollisions()
		{
			CollisionEnabled = false;
			ParsePropertyValue("Mass", "0");

			if (physComponent != nullptr)
			{
				physComponent->DisablePhysics();
			}
		}

		std::vector<std::string> material_names;
		std::vector<std::string> texture_names;
	protected:
		virtual void updateCollisions() = 0;
		glm::uvec3 bound = { 0.f, 0.f, 0.f };
		bool visibility = true;

		bool CollisionEnabled = true;
		Physics::PhysicsObject* physComponent;
	};
}
