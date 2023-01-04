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

			Color* new_prop = new Color(0.15, 0.85, 0.25, this);
			properties.push_back(new_prop);
			color_mask = static_cast<glm::vec4*>(new_prop->GetValueAdress());
			physics_object = true;
		};

		virtual ~DrawableObject()
		{
			
		};

		virtual bool LoadMesh(const char* mesh_path, bool useTexturing, std::vector<std::string>* out_materials) = 0;


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

		glm::vec4 GetColorMask()
		{
			return *color_mask;
		}

		void SetObjectScale(glm::vec3 new_scale) override
		{
			scale = new_scale;
			recalculatePhysics();
		}

		void SetObjectScale(float scalex, float scaley, float scalez) override
		{
			colShape->setLocalScaling(btVector3(scalex, scaley, scalez));
			scale = { scalex, scaley, scalez };
			recalculatePhysics();
		}

	protected:
		glm::vec4* color_mask;
		glm::uvec3 colorID = { 0, 0, 0 };
		virtual void updateCollisions() = 0;
		glm::vec3 bound = { 0.f, 0.f, 0.f };
		bool visibility = true;
		Mesh object_mesh;
	};
}
