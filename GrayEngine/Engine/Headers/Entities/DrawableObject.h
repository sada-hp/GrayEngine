#pragma once
#include <pch.h>
#include "Entity.h"

namespace GrEngine
{
	struct Texture
	{
		const char* texture_path;
		bool initialized = false;
	};

	struct Vertex
	{
		glm::vec4 pos;
		glm::vec4 color;
		glm::vec2 uv;
		uint32_t uv_index;
		uint32_t uses_texture;
		glm::uvec3 inID;

		Vertex(glm::vec4 position, glm::vec2 uv_coordinates, glm::uvec3 color_attach = {0, 0, 0}, uint32_t material_index = 0, BOOL use_texture = 0)
		{
			pos = position;
			uv = uv_coordinates;
			uv_index = material_index;
			uses_texture = use_texture;
			inID = color_attach;
		}

		bool operator==(const Vertex& other) const
		{
			return pos == other.pos && uv == other.uv;
		}
	};

	struct Mesh
	{
		const char* mesh_path = "";
	};

	class DllExport DrawableObject : public Entity
	{
	public:
		DrawableObject()
		{
			recalculatePhysics();
			Type = "Object";
			physics_object = true;
		};

		virtual ~DrawableObject()
		{
			
		};

		virtual bool LoadMesh(const char* mesh_path, bool useTexturing, std::vector<std::string>* out_materials) = 0;
		virtual bool LoadModel(const char* model_path) = 0;
		virtual bool LoadModel(const char* mesh_path, std::vector<std::string> textures_vector) = 0;

		void ParsePropertyValue(const char* property_name, const char* property_value) override
		{
			for (int i = 0; i < properties.size(); i++)
			{
				auto name = properties[i]->property_name;
				if (std::string(properties[i]->property_name) == std::string(property_name))
				{
					properties[i]->ParsePropertyValue(property_value);
					recalculatePhysics();
				}
			}
		}


		glm::vec3& GetObjectBounds()
		{
			return bound;
		}

		void SetObjectBounds(glm::vec3 new_bounds)
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

		std::vector<std::string> material_names;
		std::vector<std::string> texture_names;
	protected:
		virtual void updateCollisions() = 0;
		glm::vec3 bound = { 0.f, 0.f, 0.f };
		bool visibility = true;
	};
}
