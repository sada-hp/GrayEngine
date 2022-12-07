#pragma once
#include <pch.h>
#include "Engine/Source/Headers/Entity.h"

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

		Vertex(glm::vec4 position, glm::vec4 vertexcolor, glm::vec2 uv_coordinates, glm::uvec3 color_attach = {0, 0, 0}, uint32_t material_index = 0, BOOL use_texture = 0)
		{
			pos = position;
			color = vertexcolor;
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
		const char* mesh_path;
	};

	class DllExport DrawableObject : public Entity
	{
	public:
		DrawableObject() {};
		virtual ~DrawableObject() {};


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
	protected:
		glm::uvec3 colorID = { 0, 0, 0 };

	private:
		glm::vec3 bound = { 0.f, 0.f, 0.f };
		bool visibility = true;
	};
}
